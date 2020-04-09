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
			//将Span的后半部分做为newspan进行返回是为了减少映射关系的次数。
			Span* newspan = new Span;//newspan为分裂后的span的后半部分
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
	void* ptr = SystemAlloc(MAX_PAGES - 1);//向系统申请128页内存
	Span* bigspan = new Span;

	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->_pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->_pagesize; i++)//将申请出来内存的每一页的_pageid都映射一个Span*,刚开始的多个页号都映射同一个Span*
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}
	_spanLists[bigspan->_pagesize].PushBack(bigspan);
	return NewSpan(numpage);//此次巧妙设计调用递归,多思考
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
	//向前合并页
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;
		auto it = _idSpanMap.find(prevPageId);
		//前面的页PageCache未申请
		if (it == _idSpanMap.end())
		{
			break;
		}
		//如果_usecount != 0,说明前面的页还在使用，不能合并
		Span* prevSpan = it->second;
		if (prevSpan->_usecount == 0)
		{
			break;
		}
		//合并
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		delete prevSpan;
	}

	//向后合并
}