#define _CRT_SECURE_NO_WARNINGS 1
#include"util.h"
int main()
{
	std::vector<Adapter> list;
	AdapterUtil::GetAllAdapter(&list);
	system("pause");
	return 0;
}