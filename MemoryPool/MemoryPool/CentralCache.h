#pragma once
#include"Public.h"
#include"PageCache.h"
class CentralCache
{
public: 
	// �����Ļ����ȡһ�������Ķ����ThreadCache
	size_t GetRangeObj(void*& start, void*& end, size_t num, size_t size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start);

	// ��spanlist ���� page cache��ȡһ��span
	Span* GetOneSpan(size_t size);
private:
	SpanList _spanLists[NFREE_LIST];
};

static CentralCache centralCacheInst;//���ｫ�������Ϊstatic,��������ֻʵ��һ��CentralCache

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

	// ��PageCache ��ȡһ��span
	size_t numpage = SizeClass::NumMovePage(size);
	Span* span = pageCacheInst.NewSpan(numpage);

	//��span�����гɶ�Ӧ��С�ҵ�span��freelist��
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
