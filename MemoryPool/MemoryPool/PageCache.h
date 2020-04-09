#pragma once
#include"Public.h"
class PageCache
{
public:
	Span* NewSpan(size_t numpage); 
	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);
private:
	SpanList _spanLists[MAX_PAGES];
	map<PAGE_ID, Span*> _idSpanMap;
};

static PageCache pageCacheInst;
Span* PageCache::NewSpan(size_t numpage)
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
				_idSpanMap[newspan->_pageid] = newspan;
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

	for (PAGE_ID i = 0; i < bigspan->_pagesize; i++)//����������ڴ��ÿһҳ��_pageid��ӳ��һ��Span*,�տ�ʼ�Ķ��ҳ�Ŷ�ӳ��ͬһ��Span*
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}
	_spanLists[bigspan->_pagesize].PushBack(bigspan);
	return NewSpan(numpage);//�˴�������Ƶ��õݹ�,��˼��
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
		PAGE_ID prevPageId = span->_pageid - 1;
		auto it = _idSpanMap.find(prevPageId);
		//ǰ���ҳPageCacheδ����
		if (it == _idSpanMap.end())
		{
			break;
		}
		//���_usecount != 0,˵��ǰ���ҳ����ʹ�ã����ܺϲ�
		Span* prevSpan = it->second;
		if (prevSpan->_usecount == 0)
		{
			break;
		}
		//�ϲ�
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		delete prevSpan;
	}

	//���ϲ�
}