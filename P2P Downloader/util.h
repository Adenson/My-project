#pragma once
#ifdef _WIN32
#include<iostream>
#include<stdint.h>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息的头文件
#pragma comment(lib, "Iphlpapi.lib")//获取网卡信息接口的库文件包含
#pragma comment(lib, "ws2_32.lib")
#include<vector>
#include<fstream>
#include<boost/filesystem.hpp>
#include<sstream>
#else
//linux头文件
#endif

class StringUtil
{
public:
	static int64_t Str2Dig(const std::string& num)
	{
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;
		return res;
	}
};
//文件操作工具类
class FileUtil
{
public:
	static int64_t GetFileSize(const std::string& name)
	{
		return boost::filesystem::file_size(name);
	}

	static bool Write(const std::string& name, const std::string& body, int64_t offset = 0)//offset为偏移量
	{
		std::cout << "写入文件数据:" << name << " " << "size:" << body.size() << std::endl;
		std::ofstream ofs(name,std::ios::binary);//需要以二进制的方式
		if (ofs.is_open() == false)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		ofs.seekp(offset, std::ios::beg);//读写位置到相对于文件起始位置开始偏移offset的偏移量相当于fseek
		ofs.write(&body[0], body.size());//body[0]也就是第一个字符的地址，也就是这块空间的首地址
		if (ofs.good() == false)//判断上次向文件写入数据是否成功
		{
			std::cerr << "向文件写入数据失败\n";
			ofs.close();
			return false;
		}
		ofs.close();
		return true;
	}

	//指针参数表示这是一个输出型参数
	//const& 表示这是一个输入型参数
	//&表示这是一个输入输出型参数
	static bool Read(const std::string& name, std::string* body)
	{
		std::ifstream ifs(name, std::ios::binary);//需要以二进制的方式
		if (ifs.is_open() == false)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		//通过boost库获取文件名来获取文件大小
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << " " << "size:" << filesize << std::endl;
		ifs.read(&(*body)[0], filesize);

		if (ifs.good() == false)
		{
			//std::cout << *body << std::endl;
			std::cerr << "读取文件数据失败\n";
			ifs.close();
			return false;
		}

		ifs.close();
		return true;
	}

	//从name文件中offset位置读取len长度数据放到body中
	static bool ReadRange(const std::string& name, std::string* body, int64_t len, int64_t offset)
	{
		body->resize(len);
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "从文件中读取数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
};

class Adapter
{
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码
};

//获取网卡信息的实现
class AdapterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//获取所有网卡信息 
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//开辟一块网卡信息结构空间
		//IP_ADAPTER_INFO 是网卡信息的结构体；PIP_ADAPTER_INFO是该网卡信息结构体的指针
		uint64_t all_adapters_size = sizeof(IP_ADAPTER_INFO);
		//all_adapters_size用于获取实际一块网卡信息所占用空间大小
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		//GetAdaptersInfo 是在win下获取网卡信息的接口,获取网卡信息有可能失败，因为可能获取到不止一块网卡，所以导致空间不足，所以GetAdaptersInfo有一个输出型参数all_adapters_size用于向用户返回所当前主机所有网卡信息占用空间大小
		if (ret == ERROR_BUFFER_OVERFLOW)//说明缓冲区空间不足，当前主机不止一块网卡，需要重新给指针分配空间
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapters_size];
			ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);//重新获取网卡信息
		}

		while (p_adapters)
		{
			Adapter adapter;
			//将一个字符串点分十进制IP地址转换为网络字节序整数IP地址
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)//有些网卡没有启用导致IP地址为0
			{
				list->push_back(adapter);//将网卡信息添加到vector中返回给用户
				std::cout << "网卡名称：" << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述：" << p_adapters->Description << std::endl;
				std::cout << "IP地址：" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码：" << p_adapters->IpAddressList.IpMask.String << std::endl;
				std::cout << std::endl;
			}
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
