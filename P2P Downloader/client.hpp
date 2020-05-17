#pragma once
#include<thread>
#include"util.hpp"
#include"httplib.h"
#define P2P_PORT 9000
#define IPBUFFER 16
#define SHARED_PATH "./shared"
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
		//整个过程是使用第三方库httplib实现
		//1.组织http协议格式的请求数据
		//2.搭建一个tcp的客户端，将数据发送
		//3.等待服务器端的回复，并进行解析

		host->_pair_ret = false;
		char buf[IPBUFFER] = { 0 };
		//将网络字节序的IP地址转换为字符串点分十进制IP地址
		inet_ntop(AF_INET, &host->_ip_addr, buf, IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);
		auto rsp = cli.Get("/hostpair");//向服务端发送资源为/hostpair的GET请求//若链接建立失败Get会返回NULL
		if (rsp != NULL && rsp->status == 200)//注意需要判断rsp是否为NULL，否则会出现访问错误
		{
			host->_pair_ret = true;//链接建立成功则重置主机配对结果
		}
		return;
	}
	//获取在线主机
	bool GetOnlineHost()
	{
		//1、获取网卡信息，得到局域网中所有的IP地址列表
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
				host._ip_addr = htonl(host_ip);//将主机字节序的IP地址转换为网络字节序
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
		//2、等待所有线程主机配对完毕，判断配对结果，并将在线主机添加到online_host中
		for (int i = 0; i < host_list.size(); i++)
		{
			thr_list[i]->join();
			if (host_list[i]._pair_ret == true)
			{
				_online_host.push_back(host_list[i]);
			}
			delete thr_list[i];
		}

		//3、将所有在线主机的IP打印出来，供用户选择
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		std::cout << "请选择配对主机，获取共享文件列表:";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		//用户选择主机之后，调用获取文件列表接口
		GetShareList(select_ip);
	}

	//获取文件列表
	bool GetShareList(const std::string& host_ip)
	{
		//1.先发送请求
		//2.得到响应之后，解析正文（文件名称）
		httplib::Client cli(host_ip, P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "获取文件列表响应出错\n";
			return false;
		}
		//打印正文---打印服务端响应的文件名称列表让用户进行选择
		std::cout << rsp->body << std::endl;
		std::cout << "请选择要下载的文件:";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		DownLoadFile(host_ip, filename);
		return true;
	}
	//下载文件
	bool DownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.向服务端发送文件下载请求“/filename”
		//2.得到响应结果，响应结果中的body正文就是文件数据
		//3.创建文件，将文件数据写入文件中，关闭文件

		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip, P2P_PORT);
		auto rsp = cli.Get(req_path);
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "下载文件，获取响应信息失败";
			return false;
		}
		if (FileUtil::Write(filename, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		return true;
	}
private:
	std::vector<Host> _online_host;
};

class Server
{
public:
	bool Start()
	{
		//添加针对客户端请求的处理方式对应关系
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);

		//正则表达式：将特殊字符以指定的格式，表示具有关键特征的数据
		//.点号:表示匹配除了\n或\r之外的任意字符 *：表示匹配前边的字符任意次
		//防止与上方的请求冲突，因此在请求中加入download路径
		_srv.Get("/download/.*", Download);

		_srv.listen("0.0.0.0", P2P_PORT);
	}
private:
	//只能设置成为static，因为httplib中的函数没有this指针
	static void HostPair(const httplib::Request& req, httplib::Response& rsp)
	{
		rsp.status = 200;
		return;
	}
	//获取文件列表
	static void ShareList(const httplib::Request& req, httplib::Response& rsp)
	{

	}

	static void Download(const httplib::Request& req, httplib::Response& rsp)
	{

	}
private:
	httplib::Server _srv;
};