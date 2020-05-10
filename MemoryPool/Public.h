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
//�˴�û���ú�,��Ϊ�겻�������
const size_t MAX_SIZE = 64 * 1024;
const size_t NFREE_LIST = MAX_SIZE / 8;
const size_t MAX_PAGES = 129;
const size_t PAGE_SHIFT = 12; // ��λ��һҳΪ4k,��4*1024 = 2^12

inline void*& Next_Obj(void* obj)  //��������
{
	return  *((void**)obj);//����Ӧ32λ��64λϵͳ
}

class Freelist
{
public:
	void Push(void* obj)//ͷ��
	{
		Next_Obj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}

	void* Pop()//ͷɾ
	{
		void* obj = _freelist;
		_freelist = Next_Obj(obj);
		--_num;
		return obj;
	}

	bool Empty()//�ж�_freelist�Ƿ�Ϊ��
	{
		return _freelist == nullptr;
	}

	void pushRange(void* head, void* tail, size_t num)//ͷ�����һ�η�Χ�Ŀռ�
	{
		Next_Obj(tail) = _freelist;
		_freelist = head;
		_num += num;
	}

	//�ǵô�����
	size_t popRange(void*& start, void*& end, size_t num)//����һ�η�Χ�ڴ棬����ʵ�ʸ��˶��ٸ��ڴ��
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
		//�˴���((1 << align_shift) - 1))�����ſ������е���࣬��ʵ��Ϊ�˸��ӷ������[1,8] + 7 = [8,15]
	}

	static size_t ListIndex(const size_t size)	//��������Ŀռ�Ķ�Ӧ���±�
	{
		assert(size <= MAX_SIZE);
		static int group_array[4] = { 16, 56, 56, 56 };
		if (size <= 128)  
		{
			return _ListIndex(size, 3);//��Ϊ�˷�Χ������8�ֽڶ��룬���Դ˴��� 3 ����Ϊ 2^3 = 8,��log8
		}
		if (size <= 1024)//16�ֽڶ���
		{
			return _ListIndex(size, 4) + group_array[0];
		}
		if (size <= 8192)//128�ֽڶ���
		{
			return _ListIndex(size, 7) + group_array[0] + group_array[1];
		}
		if (size <= 65536)//1024�ֽڶ���
		{
			return _ListIndex(size, 10) + group_array[0] + group_array[1] + group_array[2];
		}
		return -1;
	}


	//����[1,8] + 7 = [8,15],[8,15]�ڼ���κ����������Զ�����λ1xxx��ʾ��1xxx &(~(alignment - 1))����
	static size_t _RoundUp(size_t size, int alignment)
	{
		return (size + alignment - 1)&(~(alignment - 1));
	}

	// ��������[1%��10%]���ҵ�����Ƭ�˷�
	// [1,128] 8byte���� freelist[0,16)
	// [129,1024] 16byte���� freelist[16,72)
	// [1025,8*1024] 128byte���� freelist[72,128)
	// [8*1024+1,64*1024] 1024byte���� freelist[128,184)
	static size_t RoundUp(size_t size)	//������Ŀռ��С����
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

	//����ThreadCache��CentralCache��ȡ���ڴ������
	static size_t NumMoveSize(const size_t size)
	{
		if (size == 0)//������size���� 0 �������ж��Ƿ�ֹ����� 0 ����
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
			//�˴��趨 num = 512 ����Ϊ 512*8 = 4K����һҳ
			num = 512;
		}
		//������ڴ��Ƚϴ���������һ�㣻������ڴ��Ƚ�С�������Ͷ�һ��
		return num;
	}

	//����CentralCache��PageCache�л�ȡ��ҳ��
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;

		npage >>= 12;//�൱�ڳ���4K
		if (npage == 0)
		{
			npage = 1;
		}
		return npage;
	}
};

// ���windows
#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif // _WIN32

//����ҳΪ��λ���ڴ���󣬱����Ƿ������ϲ�������ڴ���Ƭ
struct Span
{
	PAGE_ID _pageid = 0;//ҳ��         
	unsigned int _pagesize = 0;//ҳ������
	Freelist _freelist;//��������
	int _usecount = 0;//�����Ѿ�ʹ�õ��ڴ������
	size_t _objSize = 0;//���������ж���Ĵ�С
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

inline static void* SystemAlloc(size_t numpage)//��ϵͳ����ռ�
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