#pragma once
#include"Public.h"
#include"CentralCache.h"

class ThreadCache
{
public:
	// �����ڴ�
	void* Allocte(size_t size);
	//�ͷ��ڴ�
	void Deallocte(void* ptr, size_t size);
	// �����Ļ����ȡ����
	void* GetSpace_FromCentralCache(size_t index);
	//������������еĶ��󳬹�һ�����Ⱦ���Ҫ�ͷŵ�CentralCache
	void ListTooLong(Freelist& freelist, size_t num);
private:
	Freelist _freeLists[NFREE_LIST];
};

//�����ڴ�
void* ThreadCache::Allocte(size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		return GetSpace_FromCentralCache(SizeClass::RoundUp(size));
	}
}

void ThreadCache::ListTooLong(Freelist& freelist, size_t num)
{
	void* start = nullptr;
	void* end = nullptr;
	freelist.popRange(start, end, num); 
	Next_Obj(end) = nullptr;
	centralCacheInst.ReleaseListToSpans(start);
}
//�ͷ��ڴ�
void ThreadCache::Deallocte(void* ptr, size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	Freelist& freelist = _freeLists[index];
	freelist.Push(ptr);

	size_t num = SizeClass::NumMoveSize(size);
	if (freelist.Num() >= num)
	{
		ListTooLong(freelist, num);
	}
}


//�� _freeLists[index] Ϊ nullptr ʱ���� CentralCache ��ȡ�ռ�
void* ThreadCache::GetSpace_FromCentralCache(size_t size)
{
	size_t num = SizeClass::NumMoveSize(size);	
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = centralCacheInst.GetRangeObj(start, end, num, size);
	if (actualNum == 1)//���ֻ��ȡ����һ��ֱ�ӷ���
	{
		return start;
	}
	else
	{
		//�����ȡ��������ͷ���һ������ʣ�µĹ�����
		size_t index = SizeClass::ListIndex(size);
		Freelist& list = _freeLists[index];
		list.pushRange(Next_Obj(start), end, actualNum);
		return start;
	} 
	return nullptr;
}


//��������ThreadCache
////�� _freeLists[index] Ϊ nullptr ʱ���� CentralCache ��ȡ�ռ�
//void* ThreadCache::GetSpace_FromCentralCache(size_t index)
//{
//	size_t num = 10;
//	size_t size = (index + 1) * 8;//ͨ�� index ���� size,Ȼ���ٿ� num �� size ��С�Ŀռ�
//	char* start = (char*)malloc(size * num);
//	char* cur = start;
//	for (size_t i = 0; i < num -1; i++)
//	{
//		char* next = cur + size;//�˴�ͨ�� cur + size ������ÿһ�οռ�
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