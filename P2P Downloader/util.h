#pragma once
#ifdef _WIN32
#include<iostream>
#include<stdint.h>
#include<WS2tcpip.h>
#include<Iphlpapi.h>//��ȡ������Ϣ��ͷ�ļ�
#pragma comment(lib, "Iphlpapi.lib")//��ȡ������Ϣ�ӿڵĿ��ļ�����
#pragma comment(lib, "ws2_32.lib")
#include<vector>
#include<fstream>
#include<boost/filesystem.hpp>
#include<sstream>
#else
//linuxͷ�ļ�
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
//�ļ�����������
class FileUtil
{
public:
	static int64_t GetFileSize(const std::string& name)
	{
		return boost::filesystem::file_size(name);
	}

	static bool Write(const std::string& name, const std::string& body, int64_t offset = 0)//offsetΪƫ����
	{
		std::cout << "д���ļ�����:" << name << " " << "size:" << body.size() << std::endl;
		std::ofstream ofs(name,std::ios::binary);//��Ҫ�Զ����Ƶķ�ʽ
		if (ofs.is_open() == false)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		ofs.seekp(offset, std::ios::beg);//��дλ�õ�������ļ���ʼλ�ÿ�ʼƫ��offset��ƫ�����൱��fseek
		ofs.write(&body[0], body.size());//body[0]Ҳ���ǵ�һ���ַ��ĵ�ַ��Ҳ�������ռ���׵�ַ
		if (ofs.good() == false)//�ж��ϴ����ļ�д�������Ƿ�ɹ�
		{
			std::cerr << "���ļ�д������ʧ��\n";
			ofs.close();
			return false;
		}
		ofs.close();
		return true;
	}

	//ָ�������ʾ����һ������Ͳ���
	//const& ��ʾ����һ�������Ͳ���
	//&��ʾ����һ����������Ͳ���
	static bool Read(const std::string& name, std::string* body)
	{
		std::ifstream ifs(name, std::ios::binary);//��Ҫ�Զ����Ƶķ�ʽ
		if (ifs.is_open() == false)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		//ͨ��boost���ȡ�ļ�������ȡ�ļ���С
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "��ȡ�ļ�����:" << name << " " << "size:" << filesize << std::endl;
		ifs.read(&(*body)[0], filesize);

		if (ifs.good() == false)
		{
			//std::cout << *body << std::endl;
			std::cerr << "��ȡ�ļ�����ʧ��\n";
			ifs.close();
			return false;
		}

		ifs.close();
		return true;
	}

	//��name�ļ���offsetλ�ö�ȡlen�������ݷŵ�body��
	static bool ReadRange(const std::string& name, std::string* body, int64_t len, int64_t offset)
	{
		body->resize(len);
		FILE* fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len)
		{
			std::cerr << "���ļ��ж�ȡ����ʧ��\n";
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
	uint32_t _ip_addr;//�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ���������
};

//��ȡ������Ϣ��ʵ��
class AdapterUtil
{
#ifdef _WIN32
public:
	static bool GetAllAdapter(std::vector<Adapter>* list)//��ȡ����������Ϣ 
	{
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//����һ��������Ϣ�ṹ�ռ�
		//IP_ADAPTER_INFO ��������Ϣ�Ľṹ�壻PIP_ADAPTER_INFO�Ǹ�������Ϣ�ṹ���ָ��
		uint64_t all_adapters_size = sizeof(IP_ADAPTER_INFO);
		//all_adapters_size���ڻ�ȡʵ��һ��������Ϣ��ռ�ÿռ��С
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);
		//GetAdaptersInfo ����win�»�ȡ������Ϣ�Ľӿ�,��ȡ������Ϣ�п���ʧ�ܣ���Ϊ���ܻ�ȡ����ֹһ�����������Ե��¿ռ䲻�㣬����GetAdaptersInfo��һ������Ͳ���all_adapters_size�������û���������ǰ��������������Ϣռ�ÿռ��С
		if (ret == ERROR_BUFFER_OVERFLOW)//˵���������ռ䲻�㣬��ǰ������ֹһ����������Ҫ���¸�ָ�����ռ�
		{
			delete p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapters_size];
			ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapters_size);//���»�ȡ������Ϣ
		}

		while (p_adapters)
		{
			Adapter adapter;
			//��һ���ַ������ʮ����IP��ַת��Ϊ�����ֽ�������IP��ַ
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0)//��Щ����û�����õ���IP��ַΪ0
			{
				list->push_back(adapter);//��������Ϣ��ӵ�vector�з��ظ��û�
				std::cout << "�������ƣ�" << p_adapters->AdapterName << std::endl;
				std::cout << "����������" << p_adapters->Description << std::endl;
				std::cout << "IP��ַ��" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "�������룺" << p_adapters->IpAddressList.IpMask.String << std::endl;
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
