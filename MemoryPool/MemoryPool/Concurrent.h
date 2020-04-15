#pragma once
#include"ThreadCache.h"

void* ConcurrentMalloc(size_t size)
{
	if (size <= MAX_SIZE)//[1�ֽڣ�64KB]��threadCache����
	{
		if (tlsThreadCache == nullptr)
		{
			tlsThreadCache = new ThreadCache;
		}
		return tlsThreadCache->Allocte(size);
	}
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))//(64KB,128*4KB]��PageCache����
	{
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		Span* span = PageCache::GetInstance().NewSpan(pagenum);
		span->_objSize = align_size;
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else//(128*4KB,---]����128ҳֱ����ϵͳ��������
	{
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		return SystemAlloc(pagenum);
	}
}

void ConcurrentFree(void* ptr)
{
	size_t pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance().GetIdToSpan(pageid);

	if (span == nullptr)//spanΪnullptr˵����Ҫ�ͷŵ��ڴ���ֱ�Ӵ�ϵͳ����ģ���Ϊ��pageCache����Ķ���ӳ����_idSpanMap��
	{
		SystemFree(ptr); //(128 * 4KB, --- ]
		return;
	}
	size_t size = span->_objSize;
	if (size <= MAX_SIZE)//[1�ֽڣ�64KB]
	{
		tlsThreadCache->Deallocte(ptr, size);
	 }
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))//(64KB,128*4KB]
	{
		PageCache::GetInstance().ReleaseSpanToPageCache(span);
	}
}