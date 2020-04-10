#include"ThreadCache.h"
#include"Concurrent.h"
#include<vector>
void UnitThreadCache()
{
	ThreadCache tc;
	vector<void*> v;
	size_t size = 7;
	for (size_t i = 1; i <= 512; ++i)
	{
		v.push_back(tc.Allocte(size));
	}
	for (size_t i = 0; i < v.size(); ++i)
	{
		printf("[%d]->%p\n", i, v[i]);
	}
	for (auto ptr: v)
	{
		tc.Deallocte(ptr, size);
	}
	v.clear();
	/*ThreadCache tc;
	void* ptr1 = tc.Allocte(7);
	cout << ptr1 << endl;
	void* ptr2 = tc.Allocte(6);
	cout << ptr2 << endl;*/
}

//void UnitTestSizeClass()
//{
//	cout << SizeClass::RoundUp(1) << endl;
//	cout << SizeClass::RoundUp(127) << endl;
//	cout << endl;
//
//	cout << SizeClass::RoundUp(129) << endl;
//	cout << SizeClass::RoundUp(1023) << endl;
//	cout << endl;
//
//	cout << SizeClass::RoundUp(1025) << endl;
//	cout << SizeClass::RoundUp(8 * 1024 - 1) << endl;
//	cout << endl;
//
//	cout << SizeClass::RoundUp(8 * 1024 + 1) << endl;
//	cout << SizeClass::RoundUp(64 * 1024 - 1) << endl;
//	cout << endl << endl;
//
//	cout << SizeClass::ListIndex(1) << endl;
//	cout << SizeClass::ListIndex(128) << endl;
//	cout << endl;
//
//	cout << SizeClass::ListIndex(129) << endl;
//	cout << SizeClass::ListIndex(1023) << endl;
//	cout << endl;
//
//	cout << SizeClass::ListIndex(1025) << endl;
//	cout << SizeClass::ListIndex(8 * 1024 - 1) << endl;
//	cout << endl;
//
//	cout << SizeClass::ListIndex(8 * 1024 + 1) << endl;
//	cout << SizeClass::ListIndex(64 * 1024 - 1) << endl;
//	cout << endl;
//}
//
//void UnitTestSystemAlloc()
//{
//	void* ptr1 = SystemAlloc(MAX_PAGES - 1);
//	cout << ptr1 << endl;
//	PAGE_ID id = (PAGE_ID)ptr1 >> PAGE_SHIFT;
//	cout << id << endl;
//	void* ptr2 = (void*)(id << PAGE_SHIFT);
//	cout << ptr2 << endl;	
//}
//

void func1(size_t n)
{
	vector<void*> v;
	size_t size = 7;
	for (size_t i = 1; i <= SizeClass::NumMoveSize(size); ++i)
	{
		v.push_back(ConcurrentMalloc(size));		
	}
	//for (size_t i = 0; i < v.size(); ++i)
	//{
	//	printf("[%d]->%p\n", i, v[i]);
	//}

	for (auto ptr : v)
	{
		ConcurrentFree(ptr);
	}

	v.clear();
}

void func2(size_t n)
{
	vector<void*> v;
	size_t size = 7;
	for (size_t i = 1; i <= SizeClass::NumMoveSize(size); ++i)
	{
		v.push_back(ConcurrentMalloc(size));
	}
	//for (size_t i = 0; i < v.size(); ++i)
	//{
	//	printf("[%d]->%p\n", i, v[i]);
	//}

	for (auto ptr : v)
	{
		ConcurrentFree(ptr);
	}

	v.clear();
}
int main()
{
	//Test_ObjectPool();
	//BenchMark();
	//UnitThreadCache();
	//UnitTestSizeClass();
	//UnitTestSystemAlloc();

	//thread T1(func1, 100);
	//thread T2(func2, 100);
	//T1.join();
	//T2.join();

	ConcurrentMalloc(1 << PAGE_SHIFT);
	system("pause");
	return 0;
}