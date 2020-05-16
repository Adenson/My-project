#pragma once
#include<thread>
#include"util.hpp"
class Host
{
public:
	uint32_t _ip_addr;//Ҫ���������IP��ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ�Ϊtrue,���ʧ��Ϊfalse
};
class client
{
public:
	//������Ե��߳���ں���
	void HostPair(Host* host)
	{

	}
	//��ȡ��������
	bool GetOnlineHost()
	{
		//1.��ȡ������Ϣ���õ������������е�IP��ַ�б�
		std::vector<Adapter> list;
		AdapterUtil::GetAllAdapter(&list);
		//��ȡ��������IP��ַ����ӵ�host_list��
		std::vector<Host> host_list; 
		for (int i = 0; i < list.size(); i++)
		{
			uint32_t ip = list[i]._ip_addr;
			uint32_t mask = list[i]._mask_addr;
			//���������,�ǵý������ֽ���ת��Ϊ�����ֽ���
			uint32_t net = (ntohl(ip&mask));
			//�������������,�ǵý������ֽ���ת��Ϊ�����ֽ���
			uint32_t max_host = (~ntohl(mask));
			for (int j = 1; j < max_host; j++)
			{
				uint32_t host_ip = net + j;
				Host host;
				host._ip_addr = host_ip;
				host._pair_ret = false;
				host_list.push_back(host);
			}
		}
		//��host_list�е����������߳̽������
		std::vector<std::thread*> thr_list(host_list.size());
		for (int i = 0; i < host_list.size(); i++)
		{
			thr_list[i] = new std::thread(&HostPair, this, &host_list[i]);
		}
		//�ȴ������߳����������ϣ��ж���Խ������������������ӵ�online_host��
		for (int i = 0; i < host_list.size(); i++)
		{
			thr_list[i]->join();
			if (host_list[i]._pair_ret == true)
			{
				_online_host.push_back(host_list[i]);
			}
			delete thr_list[i];
		}
	}
private:
	std::vector<Host> _online_host;
};