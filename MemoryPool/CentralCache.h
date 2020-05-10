#pragma once
#include"Public.h"
#include"PageCache.h"
//设计为单例模式
class CentralCache
{
public: 
	// 从中心缓存获取一定数量的对象给ThreadCache
	size_t GetRangeObj(void*& start, void*& end, size_t num, size_t size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start,size_t size);

	// 从spanlist 或者 page cache获取一个span
	Span* GetOneSpan(size_t size);

	static CentralCache& GetInstance()
	{
		static CentralCache Inst;
		return Inst;
	}
private:
	CentralCache()
	{}
	CentralCache(const CentralCache& CentralCache) = delete;
	SpanList _spanLists[NFREE_LIST];
};


size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];

	spanlist.Lock();//加锁

	Span* span = CentralCache::GetOneSpan(size);
	size_t acualNum = span->_freelist.popRange(start, end, num);
	span->_usecount += acualNum;

	spanlist.Unlock();//解锁
	return acualNum;
}

Span* CentralCache::GetOneSpan(size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())
		{
			return it;
		}			
		else
		{
			it = it->_next;
		} 
	}

	// 从PageCache 获取一个span
	size_t numpage = SizeClass::NumMovePage(size);
	Span* span = PageCache::GetInstance().NewSpan(numpage);

	//将span对象切成对应大小挂到span的freelist中
	char* start = (char*)(span->_pageid << 12);
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start; 
		start += size;
		span->_freelist.Push(obj);
	}
	span->_objSize = size;
	spanlist.PushFront(span);
	return span;
}

void CentralCache::ReleaseListToSpans(void* start,size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];

	spanlist.Lock();

	while (start)
	{
		void* next = Next_Obj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = PageCache::GetInstance().GetIdToSpan(id);//通过_idSpanMap找到每一个内存块的span
		span->_freelist.Push(start);
		span->_usecount--;

		//如果_usecount == 0,表示当前span切出去的对象全部返回，可以将span还给PageCache。
		if (span->_usecount == 0)
		{
			size_t index = SizeClass::ListIndex(span->_objSize);
			_spanLists[index].Erase(span);
			span->_freelist.Clear();
			PageCache::GetInstance().ReleaseSpanToPageCache(span);
		}
		start = next;
	}

	spanlist.Unlock();
}
