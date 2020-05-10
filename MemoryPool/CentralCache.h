#pragma once
#include"Public.h"
#include"PageCache.h"
//���Ϊ����ģʽ
class CentralCache
{
public: 
	// �����Ļ����ȡһ�������Ķ����ThreadCache
	size_t GetRangeObj(void*& start, void*& end, size_t num, size_t size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start,size_t size);

	// ��spanlist ���� page cache��ȡһ��span
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

	spanlist.Lock();//����

	Span* span = CentralCache::GetOneSpan(size);
	size_t acualNum = span->_freelist.popRange(start, end, num);
	span->_usecount += acualNum;

	spanlist.Unlock();//����
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
	Span* span = PageCache::GetInstance().NewSpan(numpage);

	//��span�����гɶ�Ӧ��С�ҵ�span��freelist��
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
		Span* span = PageCache::GetInstance().GetIdToSpan(id);//ͨ��_idSpanMap�ҵ�ÿһ���ڴ���span
		span->_freelist.Push(start);
		span->_usecount--;

		//���_usecount == 0,��ʾ��ǰspan�г�ȥ�Ķ���ȫ�����أ����Խ�span����PageCache��
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
