#pragma once
#ifdef _WIN32
#include<iostream>
#include<stdint.h>
#include<WS2tcpip.h>
#include<Iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include<vector>
#else
//linux�ļ�
#endif

class Adapter
{
public:
	uint32_t _ip_addr;//�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ���������
};

class AdapterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//windows�»�ȡ������Ϣ��ʵ��
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//IP_ADAPTER_INFO �Ǵ��������Ϣ�Ľṹ�壻PIP_ADAPTER_INFO�Ǹýṹ��ָ��
		//����� GetAdaptersInfo ����win�»�ȡ������Ϣ�Ľӿڣ���ȡ��������Ϣ�����ж�������Դ���һ��ָ�루PIP_ADAPTER_INFO��
		uint64_t all_adapters_size = sizeof(IP_ADAPTER_INFO);
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		//GetAdaptersInfo ����win�»�ȡ������Ϣ�Ľӿ�,��ȡ������Ϣ�п���ʧ�ܣ���Ϊ�ռ䲻�㣬������һ������Ͳ������������û���������������Ϣռ�ÿռ��С
		if (ret == ERROR_BUFFER_OVERFLOW)//˵���������ռ䲻�㣬��Ҫ���¸�ָ�����ռ�
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapters_size];
			ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		}

		while (p_adapters)
		{
			std::cout << "�������ƣ�" << p_adapters->AdapterName << std::endl;
			std::cout << "����������" << p_adapters->Description << std::endl;
			std::cout << "IP��ַ��" << p_adapters->IpAddressList.IpAddress.String << std::endl;
			std::cout << "�������룺" << p_adapters->IpAddressList.IpMask.String << std::endl;
			std::cout << "IP��ַ��" << p_adapters->AdapterName << std::endl;
			std::cout << std::endl;
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
public:
	static bool GetAllAdapter(std::vector<Adapter>* list);
#endif
};
