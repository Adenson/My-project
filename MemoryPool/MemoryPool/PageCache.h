#pragma once
#include"Public.h"
//����ģʽ
class PageCache
{
public:
	Span* NewSpan(size_t numpage); 
	Span* _NewSpan(size_t numpage);
	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);

	static PageCache& GetInstance()
	{
		static PageCache Inst;
		return Inst;
	}
	
private:
	SpanList _spanLists[MAX_PAGES];
	map<PAGE_ID, Span*> _idSpanMap;
	mutex _mtx;

	PageCache()
	{}
	PageCache(const PageCache& PageCache) = delete;
};

Span* PageCache::_NewSpan(size_t numpage)
{
	if (!_spanLists[numpage].Empty())
	{
		Span* span = _spanLists[numpage].Begin();
		_spanLists[numpage].PopFront();
		return span;
	}
	for (size_t i = numpage + 1; i < MAX_PAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			Span* span = _spanLists[i].Begin();
			_spanLists[i].PopFront();
			//��Span�ĺ�벿����Ϊnewspan���з�����Ϊ�˼���ӳ���ϵ�Ĵ�����
			Span* newspan = new Span;//newspanΪ���Ѻ��span�ĺ�벿��
			newspan->_pageid = span->_pageid + span->_pagesize - numpage;
			newspan->_pagesize = numpage;
			for (PAGE_ID i = 0; i < numpage; i++)
			{
				_idSpanMap[newspan->_pageid + i] = newspan;
			}
			span->_pagesize -= numpage;
			_spanLists[span->_pagesize].PushFront(span);
			return newspan;
		}
	}
	void* ptr = SystemAlloc(MAX_PAGES - 1);//��ϵͳ����128ҳ�ڴ�
	Span* bigspan = new Span;

	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->_pagesize = MAX_PAGES - 1;

	//����������ڴ��ÿһҳ��_pageid��ӳ��һ��Span*,�տ�ʼ�Ķ��ҳ�Ŷ�ӳ��ͬһ��Span*
	for (PAGE_ID i = 0; i < bigspan->_pagesize; i++)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}
	_spanLists[bigspan->_pagesize].PushBack(bigspan);
	return _NewSpan(numpage);//�˴�������Ƶ��õݹ�,��˼��
}

//Ϊ�˷�ֹ�ݹ���
Span* PageCache::NewSpan(size_t numpage)
{
	_mtx.lock();
	Span* span = _NewSpan(numpage);
	_mtx.unlock();

	return span;
}
Span* PageCache::GetIdToSpan(PAGE_ID id)
{
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//��ǰ�ϲ�ҳ
	while (1)
	{
		PAGE_ID prevPageID = span->_pageid - 1;
		auto prevIt = _idSpanMap.find(prevPageID);
		//ǰ���ҳPageCacheδ����
		if (prevIt == _idSpanMap.end())
		{
			break;
		}
		//���_usecount != 0,˵��ǰ���ҳ����ʹ�ã����ܺϲ�
		Span* prevSpan = prevIt->second;
		if (prevSpan->_usecount == 0)
		{
			break; 
		}
		//�ϲ�
		//����ϲ�����128ҳ��span����ô�Ͳ�Ҫ�ϲ���
		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		_spanLists[prevSpan->_pagesize].Erase(prevSpan);
		delete prevSpan;
	}

	//���ϲ�ҳ
	while (1)
	{
		PAGE_ID nextPageID = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageID);
		//�����ҳPageCacheδ����
		if (nextIt == _idSpanMap.end())
		{
			break;
		}
		//���_usecount != 0,˵�������ҳ����ʹ�ã����ܺϲ�
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)
		{
			break;
		}
		//����ϲ�����128ҳ��span����ô�Ͳ�Ҫ�ϲ���
		if (span->_pagesize + nextSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}
		span->_pagesize += nextSpan->_pagesize;
		for (PAGE_ID i = 0; i < nextSpan->_pagesize; ++i)
		{
			_idSpanMap[nextSpan->_pageid + i] = span;
		}
		_spanLists[nextSpan->_pagesize].Erase(nextSpan);
		delete nextSpan;
	}
		_spanLists[span->_pagesize].PushFront(span);
}