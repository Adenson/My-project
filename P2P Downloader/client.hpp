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
	uint32_t _ip_addr;//Ҫ���������IP��ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ�Ϊtrue,���ʧ��Ϊfalse
};
class client
{
public:
	//������Ե��߳���ں���
	void HostPair(Host* host)
	{
		//����������ʹ�õ�������httplibʵ��
		//1.��֯httpЭ���ʽ����������
		//2.�һ��tcp�Ŀͻ��ˣ������ݷ���
		//3.�ȴ��������˵Ļظ��������н���

		host->_pair_ret = false;
		char buf[IPBUFFER] = { 0 };
		//�������ֽ����IP��ַת��Ϊ�ַ������ʮ����IP��ַ
		inet_ntop(AF_INET, &host->_ip_addr, buf, IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);
		auto rsp = cli.Get("/hostpair");//�����˷�����ԴΪ/hostpair��GET����//�����ӽ���ʧ��Get�᷵��NULL
		if (rsp != NULL && rsp->status == 200)//ע����Ҫ�ж�rsp�Ƿ�ΪNULL���������ַ��ʴ���
		{
			host->_pair_ret = true;//���ӽ����ɹ�������������Խ��
		}
		return;
	}
	//��ȡ��������
	bool GetOnlineHost()
	{
		//1����ȡ������Ϣ���õ������������е�IP��ַ�б�
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
				host._ip_addr = htonl(host_ip);//�������ֽ����IP��ַת��Ϊ�����ֽ���
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
		//2���ȴ������߳����������ϣ��ж���Խ������������������ӵ�online_host��
		for (int i = 0; i < host_list.size(); i++)
		{
			thr_list[i]->join();
			if (host_list[i]._pair_ret == true)
			{
				_online_host.push_back(host_list[i]);
			}
			delete thr_list[i];
		}

		//3������������������IP��ӡ���������û�ѡ��
		for (int i = 0; i < _online_host.size(); i++)
		{
			char buf[IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		std::cout << "��ѡ�������������ȡ�����ļ��б�:";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		//�û�ѡ������֮�󣬵��û�ȡ�ļ��б�ӿ�
		GetShareList(select_ip);
	}

	//��ȡ�ļ��б�
	bool GetShareList(const std::string& host_ip)
	{
		//1.�ȷ�������
		//2.�õ���Ӧ֮�󣬽������ģ��ļ����ƣ�
		httplib::Client cli(host_ip, P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ��б���Ӧ����\n";
			return false;
		}
		//��ӡ����---��ӡ�������Ӧ���ļ������б����û�����ѡ��
		std::cout << rsp->body << std::endl;
		std::cout << "��ѡ��Ҫ���ص��ļ�:";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		DownLoadFile(host_ip, filename);
		return true;
	}
	//�����ļ�
	bool DownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.�����˷����ļ���������/filename��
		//2.�õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�����д���ļ��У��ر��ļ�

		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip, P2P_PORT);
		auto rsp = cli.Get(req_path);
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "�����ļ�����ȡ��Ӧ��Ϣʧ��";
			return false;
		}
		if (FileUtil::Write(filename, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
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
		//�����Կͻ�������Ĵ���ʽ��Ӧ��ϵ
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);

		//������ʽ���������ַ���ָ���ĸ�ʽ����ʾ���йؼ�����������
		//.���:��ʾƥ�����\n��\r֮��������ַ� *����ʾƥ��ǰ�ߵ��ַ������
		//��ֹ���Ϸ��������ͻ������������м���download·��
		_srv.Get("/download/.*", Download);

		_srv.listen("0.0.0.0", P2P_PORT);
	}
private:
	//ֻ�����ó�Ϊstatic����Ϊhttplib�еĺ���û��thisָ��
	static void HostPair(const httplib::Request& req, httplib::Response& rsp)
	{
		rsp.status = 200;
		return;
	}
	//��ȡ�ļ��б�
	static void ShareList(const httplib::Request& req, httplib::Response& rsp)
	{

	}

	static void Download(const httplib::Request& req, httplib::Response& rsp)
	{

	}
private:
	httplib::Server _srv;
};