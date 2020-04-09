#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
using namespace std;
#include<string>
#include<vector>
#include<time.h>

template<class t, size_t initnum = 100>
class objectpool
{
public:
	objectpool()
	{
		_itemsize = sizeof(t) < sizeof(t*) ? sizeof(t*) : sizeof(t);
		_start = (char*)malloc(initnum*_itemsize);
		_end = _start + initnum*_itemsize;
		_freelist = nullptr;
	}

	t*& next_obj(t* obj)
	{
		return  *(t**)obj;
	}

	t* new()
	{
		t* obj = nullptr;
		//如果_freelist里面有对象，则优先取出，如果没有就池里去取
		if (_freelist != nullptr)
		{
			obj = _freelist;
			_freelist = next_obj(obj);
		}
		else
		{
			if (_start == _end)
			{
				_start = (char*)malloc(initnum*_itemsize);
				_end = _start + initnum * _itemsize;
			}
			obj = (t*)_start;
			_start += _itemsize;

		}
		new(obj)t;
		return obj;
	}

	void detele(t* ptr)
	{
		ptr->~t();

		next_obj(ptr) = _freelist;
		_freelist = ptr;
	}

private:
	char* _start;
	char* _end;
	size_t _itemsize;
	t* _freelist;//自由链表
};

void test_objectpool()
{
	objectpool<int> pool;
	int* p1 = pool.new();
	int* p2 = pool.new();
	int* p3 = pool.new();
	pool.detele(p1);
	pool.detele(p2);
	pool.detele(p3);
	cout << p1 << endl;
	cout << p2 << endl;
	cout << p3 << endl;
	cout << endl;

	int* p4 = pool.new();
	int* p5 = pool.new();
	int* p6 = pool.new();
	pool.detele(p4);
	pool.detele(p5);
	pool.detele(p6);
	cout << p4 << endl;
	cout << p5 << endl;
	cout << p6 << endl;
}

void benchmark()
{
	size_t n = 10000;

	vector<string*> v1;
	v1.reserve(n);
	size_t begin1 = clock();

	for (size_t i = 0; i < n; ++i)
	{
		v1.push_back(new string);
	}

	for (size_t i = 0; i < n; ++i)
	{
		delete v1[i];
	}

	size_t end1 = clock();

	v1.clear();

	cout << "直接系统申请内存:" << end1 - begin1 << endl;

	vector<string*> v2;
	v2.reserve(n);
	objectpool<string> pool;

	size_t begin2 = clock();

	for (size_t i = 0; i < n; ++i)
	{
		v2.push_back(pool.new());
	}

	for (size_t i = 0; i < n; ++i)
	{
		pool.detele(v2[i]);
	}

	size_t end2 = clock();

	v2.clear();

	cout << "内存池申请内存:" << end2 - begin2 << endl;
}