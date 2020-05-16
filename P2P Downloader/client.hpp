#pragma once
#include<thread>
#include"util.hpp"
class Host
{
public:
	uint32_t _ip_addr;//要配对主机的IP地址
	bool _pair_ret;//用于存放配对结果，配对成功为true,配对失败为false
};
class client
{
public:
	//主机配对的线程入口函数
	void HostPair(Host* host)
	{

	}
	//获取在线主机
	bool GetOnlineHost()
	{
		//1.获取网卡信息，得到局域网中所有的IP地址列表
		std::vector<Adapter> list;
		AdapterUtil::GetAllAdapter(&list);
		//获取所有主机IP地址，添加到host_list中
		std::vector<Host> host_list; 
		for (int i = 0; i < list.size(); i++)
		{
			uint32_t ip = list[i]._ip_addr;
			uint32_t mask = list[i]._mask_addr;
			//计算网络号,记得将网络字节序转换为主机字节序
			uint32_t net = (ntohl(ip&mask));
			//计算最大主机号,记得将网络字节序转换为主机字节序
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
		//对host_list中的主机创建线程进行配对
		std::vector<std::thread*> thr_list(host_list.size());
		for (int i = 0; i < host_list.size(); i++)
		{
			thr_list[i] = new std::thread(&HostPair, this, &host_list[i]);
		}
		//等待所有线程主机配对完毕，判断配对结果，并将在线主机添加到online_host中
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