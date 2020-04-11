#pragma once
#include"Public.h"
#include"CentralCache.h"

class ThreadCache
{
public:
	// 申请内存
	void* Allocte(size_t size);
	//释放内存
	void Deallocte(void* ptr, size_t size);
	// 从中心缓存获取对象
	void* GetSpace_FromCentralCache(size_t index);
	//如果自由链表中的对象超过一定长度就需要释放到CentralCache
	void ListTooLong(Freelist& freelist, size_t num,size_t size);
private:
	Freelist _freeLists[NFREE_LIST];
};

//线程TLS
//一个线程内部的各个函数调用都能访问、但其它线程不能访问的变量（被称为static memory local to a thread 线程局部静态变量),也就是TLS
_declspec (thread) static ThreadCache* tlsThreadCache = nullptr;

//申请内存
void* ThreadCache::Allocte(size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	Freelist& freeList = _freeLists[index];
	if (!freeList.Empty())
	{
		return freeList.Pop();
	}
	else
	{
		return GetSpace_FromCentralCache(SizeClass::RoundUp(size));
	}
}

void ThreadCache::ListTooLong(Freelist& freelist, size_t num, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	freelist.popRange(start, end, num); 
	Next_Obj(end) = nullptr;
	CentralCache::GetInstance().ReleaseListToSpans(start, size);
}
//释放内存
void ThreadCache::Deallocte(void* ptr, size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	Freelist& freelist = _freeLists[index];
	freelist.Push(ptr);

	size_t num = SizeClass::NumMoveSize(size);
	if (freelist.Num() >= num)
	{
		ListTooLong(freelist, num, size);
	}
}


//当 _freeLists[index] 为 nullptr 时，从 CentralCache 中取空间
void* ThreadCache::GetSpace_FromCentralCache(size_t size)
{
	size_t num = SizeClass::NumMoveSize(size);	
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance().GetRangeObj(start, end, num, size);
	if (actualNum == 1)//如果只获取到就一个直接返回
	{
		return start;
	}
	else
	{
		//如果获取到多个，就返回一个，将剩下的挂起来
		size_t index = SizeClass::ListIndex(size);
		Freelist& list = _freeLists[index];
		list.pushRange(Next_Obj(start), end, actualNum);
		return start;
	} 
	return nullptr;
}









//独立测试ThreadCache
////当 _freeLists[index] 为 nullptr 时，从 CentralCache 中取空间
//void* ThreadCache::GetSpace_FromCentralCache(size_t index)
//{
//	size_t num = 10;
//	size_t size = (index + 1) * 8;//通过 index 计算 size,然后再开 num 个 size 大小的空间
//	char* start = (char*)malloc(size * num);
//	char* cur = start;
//	for (size_t i = 0; i < num -1; i++)
//	{
//		char* next = cur + size;//此处通过 cur + size 决定了每一段空间
//		Next_Obj(cur) = next;
//		cur = next;
//	}
//	Next_Obj(cur) = nullptr;
//
//	void* head = Next_Obj(start);
//	void* tail = cur;
//
//	_freeLists[index].pushRange(head, tail);
//	return start;
//}