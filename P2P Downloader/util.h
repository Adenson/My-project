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
//linux文件
#endif

class Adapter
{
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

class AdapterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//windows下获取网卡信息的实现
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//IP_ADAPTER_INFO 是存放网卡信息的结构体；PIP_ADAPTER_INFO是该结构体指针
		//下面的 GetAdaptersInfo 是在win下获取网卡信息的接口，获取的网卡信息可能有多个，所以传入一个指针（PIP_ADAPTER_INFO）
		uint64_t all_adapters_size = sizeof(IP_ADAPTER_INFO);
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		//GetAdaptersInfo 是在win下获取网卡信息的接口,获取网卡信息有可能失败，因为空间不足，所以有一个输出型参数，用于向用户返回所有网卡信息占用空间大小
		if (ret == ERROR_BUFFER_OVERFLOW)//说明缓冲区空间不足，需要重新给指针分配空间
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapters_size];
			ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		}

		while (p_adapters)
		{
			std::cout << "网卡名称：" << p_adapters->AdapterName << std::endl;
			std::cout << "网卡描述：" << p_adapters->Description << std::endl;
			std::cout << "IP地址：" << p_adapters->IpAddressList.IpAddress.String << std::endl;
			std::cout << "子网掩码：" << p_adapters->IpAddressList.IpMask.String << std::endl;
			std::cout << "IP地址：" << p_adapters->AdapterName << std::endl;
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
