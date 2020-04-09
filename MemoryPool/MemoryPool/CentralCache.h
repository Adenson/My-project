#pragma once
#include"Public.h"
#include"PageCache.h"
class CentralCache
{
public: 
	// 从中心缓存获取一定数量的对象给ThreadCache
	size_t GetRangeObj(void*& start, void*& end, size_t num, size_t size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start);

	// 从spanlist 或者 page cache获取一个span
	Span* GetOneSpan(size_t size);
private:
	SpanList _spanLists[NFREE_LIST];
};

static CentralCache centralCacheInst;//这里将对象定义成为static,这样可以只实现一个CentralCache

size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	Span* span = CentralCache::GetOneSpan(size);
	size_t acualNum = span->_freelist.popRange(start, end, num);
	span->_usecount += acualNum;
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
	Span* span = pageCacheInst.NewSpan(numpage);

	//将span对象切成对应大小挂到span的freelist中
	char* start = (char*)(span->_pageid << 12);
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start; 
		start += size;
		span->_freelist.Push(obj);
	}
	spanlist.PushFront(span);
	return span;
}

void CentralCache::ReleaseListToSpans(void* start)
{
	while (start)
	{
		void* next = Next_Obj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = pageCacheInst.GetIdToSpan(id);
		span->_freelist.Push(start);
		span->_usecount--;

		if (span->_usecount == 0)
		{
			_
		}
	}
}
