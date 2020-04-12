#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<unordered_map>
#include<thread>
#include<mutex>
#ifdef _WIN32
#include<Windows.h>
#endif
using namespace std;
//此处没有用宏,因为宏不方便调试
const size_t MAX_SIZE = 64 * 1024;
const size_t NFREE_LIST = MAX_SIZE / 8;
const size_t MAX_PAGES = 129;
const size_t PAGE_SHIFT = 12; // 移位，一页为4k,即4*1024 = 2^12

inline void*& Next_Obj(void* obj)  //内敛函数
{
	return  *((void**)obj);//自适应32位和64位系统
}

class Freelist
{
public:
	void Push(void* obj)//头插
	{
		Next_Obj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}

	void* Pop()//头删
	{
		void* obj = _freelist;
		_freelist = Next_Obj(obj);
		--_num;
		return obj;
	}

	bool Empty()//判断_freelist是否为空
	{
		return _freelist == nullptr;
	}

	void pushRange(void* head, void* tail, size_t num)//头插插入一段范围的空间
	{
		Next_Obj(tail) = _freelist;
		_freelist = head;
		_num += num;
	}

	//记得传引用
	size_t popRange(void*& start, void*& end, size_t num)//弹出一段范围内存，返回实际给了多少个内存块
	{
		size_t actualNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (actualNum; actualNum < num && cur != nullptr; actualNum++)
		{
			prev = cur;
			cur = Next_Obj(cur);
		}
		start = _freelist;
		end = prev;
		_freelist = cur;
		_num -= actualNum;
		return actualNum;
	}

	size_t Num()
	{
		return _num;
	}

	void Clear()
	{
		_freelist = nullptr;
		_num = 0;
	}
private:
	void* _freelist = nullptr;
	size_t _num = 0;
};

class SizeClass
{
public:
	//[1,8] + 7 = [8,15]
	static size_t _ListIndex(const size_t size, size_t align_shift)
	{
		return ((size + ((1 << align_shift) - 1)) >> align_shift) - 1;
		//此处对((1 << align_shift) - 1))加括号看起来有点多余，其实是为了更加方便理解[1,8] + 7 = [8,15]
	}

	static size_t ListIndex(const size_t size)	//计算申请的空间的对应的下标
	{
		assert(size <= MAX_SIZE);
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)  
		{
			return _ListIndex(size, 3);//因为此范围内是以8字节对齐，所以此处的 3 是因为 2^3 = 8,即log8
		}
		if (size <= 1024)//16字节对齐
		{
			return _ListIndex(size, 4) + group_array[0];
		}
		if (size <= 8192)//128字节对齐
		{
			return _ListIndex(size, 7) + group_array[0] + group_array[1];
		}
		if (size <= 65536)//1024字节对齐
		{
			return _ListIndex(size, 10) + group_array[0] + group_array[1] + group_array[2];
		}
		return -1;
	}


	//假设[1,8] + 7 = [8,15],[8,15]期间的任何数都可以以二进制位1xxx表示，1xxx &(~(alignment - 1))对齐
	static size_t _RoundUp(size_t size, int alignment)
	{
		return (size + alignment - 1)&(~(alignment - 1));
	}

	// 最多控制在[1%，10%]左右的内碎片浪费
	// [1,128] 8byte对齐 freelist[0,16)
	// [129,1024] 16byte对齐 freelist[16,72)
	// [1025,8*1024] 128byte对齐 freelist[72,128)
	// [8*1024+1,64*1024] 1024byte对齐 freelist[128,184)
	static size_t RoundUp(size_t size)	//将申请的空间大小对齐
	{
		assert(size <= MAX_SIZE);
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		if (size <= 8192)
		{
			return _RoundUp(size, 128);
		} 
		if (size <= 65536)
		{
			return _RoundUp(size, 1024);
		}
		return -1;
	}

	//计算ThreadCache从CentralCache获取的内存块数量
	static size_t NumMoveSize(const size_t size)
	{
		if (size == 0)//理论上size不会 0 ，这里判断是防止下面除 0 错误
		{
			return 0;
		}
		int num = MAX_SIZE / size;
		if (num < 2)
		{
			num = 2;
		}
		if (num > 512)
		{
			//此处设定 num = 512 是因为 512*8 = 4K，即一页
			num = 512;
		}
		//申请的内存块比较大，数量就少一点；申请的内存块比较小，数量就多一点
		return num;
	}

	//计算CentralCache从PageCache中获取的页数
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;

		npage >>= 12;//相当于除以4K
		if (npage == 0)
		{
			npage = 1;
		}
		return npage;
	}
};

// 针对windows
#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif // _WIN32

//管理页为单位的内存对象，本质是方便做合并，解决内存碎片
struct Span
{
	PAGE_ID _pageid = 0;//页号         
	unsigned int _pagesize = 0;//页的数量
	Freelist _freelist;//自由链表
	int _usecount = 0;//计算已经使用的内存块数量
	size_t _objSize = 0;//自由链表中对象的大小
	Span* _prev = nullptr;
	Span* _next = nullptr;
};

class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	
	void PushFront(Span* newspan)
	{
		Insert(_head->_next, newspan);
	}

	void PopFront()
	{
		Erase(_head->_next);
	}

	void PushBack(Span* newspan)
	{
		Insert(_head, newspan);
	}

	bool Empty()
	{
		return Begin() == End();
	}

	void PopBack()
	{
		Erase(_head->_prev);
	}

	void Insert(Span* pos, Span* newspan)
	{
		Span* prev = pos->_prev;

		prev->_next = newspan;
		newspan->_next = pos;
		pos->_prev = newspan;
		newspan->_prev = prev;
	}

	void Erase(Span* pos)
	{
		assert(_head != pos);
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	Span* Begin()
	{
		return _head->_next;
	}
	 
	Span* End()
	{
		return _head;
	}

	void Lock()
	{
		_mtx.lock();
	}

	void Unlock()
	{
		_mtx.unlock();
	}
private:
	Span* _head;
	mutex _mtx;
};

inline static void* SystemAlloc(size_t numpage)//从系统申请空间
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else

#endif
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr,0,MEM_RELEASE);
#else

#endif
}