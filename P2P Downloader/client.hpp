#pragma once
#include<thread>
#include"util.hpp"
#include"httplib.h"
#include<boost/filesystem.hpp> 
#define P2P_PORT 9000
#define IPBUFFER 16
#define SHARED_PATH "./shared/"
#define DOWNLOAD_PATH "./download/"
#define MAX_RANGE (100*1024*1024)
class Host
{
public:
	uint32_t _ip_addr;//Ҫ���������IP��ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ�Ϊtrue,���ʧ��Ϊfalse 
};
class Client
{
public:
	bool Start()
	{
		//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ�����ֻ����һ��
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
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
		//û��Ҫÿ�ζ���Ҫ������������������������֮ǰ��Ҫ���м��
		char ch = 'Y';
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ�����ƥ����������(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ��...\n";
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
				for (int j = 1; j < (int32_t)max_host; j++)
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
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "��������ƥ���У����Ե�һ���......\n";
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

		return true;
	} 

	//��ȡ�ļ��б�
	bool GetShareList(const std::string& host_ip)
	{
		//1.�ȷ�������
		//2.�õ���Ӧ֮�󣬽������ģ��ļ����ƣ�
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
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
		RangeDownLoadFile(host_ip, filename);
		return true;
	}

	//�����ļ������ļ�һ���Խ����µĻ�����������ļ���Ƚ�Σ�գ�
	bool DownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.�����˷����ļ���������/filename��
		//2.�õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�����д���ļ��У��ر��ļ�

		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "�����ļ�����ȡ��Ӧ��Ϣʧ��\n";
			return false;
		}
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		std::cout << "�Ѿ��ɹ������ļ�������" << std::endl;
		return true;
	}

	//�ֿ�����
	bool RangeDownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.����HEAD����ͨ����Ӧ�е�Content-Length��ȡ�ļ���С
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str()); 
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "��ȡ�ļ���С��Ϣʧ��\n";
			return false;
		}
		//ͨ��HTTPͷ���ֶ�����ȡֵ
		std::string clen = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::Str2Dig(clen);
		//2.�����ļ���С���зֿ�
		  //���ļ���С < ���С����ֱ�������ļ�
		if (filesize < MAX_RANGE)
		{
			return DownLoadFile(host_ip, filename);
		}
		  //���ļ���С�����������С����ֿ����Ϊ�ļ���С���Էֿ��С+1
		  //���ļ���С�պ��������С���õ���Ӧ֮��д���ļ���ָ��λ��
		int range_count = 0;
		if (filesize % MAX_RANGE == 0)
		{
			range_count = filesize / MAX_RANGE;
		}
		else{
			range_count = (filesize / MAX_RANGE) + 1;
		}

		int range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++)
		{
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1)){
				range_end = filesize - 1;
			}
			else{
				range_end = ((i + 1)*MAX_RANGE) - 1;
			}

			//3.��һ����ֿ����ݣ��õ���Ӧ֮��д���ļ���ָ��λ��
			httplib::Headers header; 
			header.insert(httplib::make_range_header({ { range_start, range_end } })); //����һ��range����
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "���������ļ�ʧ��\n";
				return false;
			}
			FileUtil::Write(filename, rsp->body, range_start);
		}
		std::cout << "�ļ����سɹ�\n";
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
		//��httplib�У�Get�ӿڲ�����Կͻ��˵�GET���󷽷���Ҳ��Կͻ��˵�HEAD���󷽷���ֻ��Ҫ��Downlowd�����н����жϾͿ���

		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	//ֻ�����ó�Ϊstatic����Ϊhttplib�еĺ���û��thisָ��
	static void HostPair(const httplib::Request& req, httplib::Response& rsp)
	{
		rsp.status = 200;
		return;
	}

	//��ȡ�ļ��б�������������һ������Ŀ¼���������Ŀ¼�µ��ļ�����Ҫ��������˵ģ�
	static void ShareList(const httplib::Request& req, httplib::Response& rsp)
	{
		//�鿴Ŀ¼�Ƿ���ڣ���Ŀ¼�����ڣ��򴴽����Ŀ¼
		if (!boost::filesystem::exists(SHARED_PATH))
		{
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//ʵ����Ŀ¼������
		boost::filesystem::directory_iterator end;//ʵ����Ŀ¼��������ĩβ
		for (; begin != end; ++begin)
		{
			//bigin->status() Ŀ¼�е�ǰ�ļ���״̬��Ϣ
			//boost::filesystem::is_directory()�жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
			if (boost::filesystem::is_directory(begin->status())){
				//�˴���ֻ�����˻�ȡ��ͨ�ļ����ƣ�û�ж�㼶Ŀ¼�Ĳ���
				continue;
			}
			else{
				std::string name = begin->path().filename().string();//�����ص��ļ����ó�Ϊ������·����
				rsp.body += name + "\r\n";
			}
		}
		rsp.status = 200;
		return;
	}

	static void Download(const httplib::Request& req, httplib::Response& rsp)
	{
		//req.path(�ͻ����������Դ·��,�����ͻ���ǰ��ӵ���download�����磺/download/filename.txt��
		boost::filesystem::path req_path(req.path);
		//�˴�ֻ��ȡ�ļ�����:filename.txt
		std::string name = req_path.filename().string();
		//ʵ���ļ���·��Ӧ�����ڹ����Ŀ¼��
		std::string realpath = SHARED_PATH + name;
		//�ж��ļ��Ƿ����
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;
			return;
		}

		if (req.method == "GET")//��Կͻ���GET���󷽷�
		{ 
			//GET���󣬾���ֱ�����������ļ������������˷ֿ鴫��Ĺ��ܺ󣬾���Ҫ�ж��Ƿ��Ƿֿ鴫������ݣ���������������Ƿ���Rangeͷ���ֶ�
			if (req.has_header("Range"))//�ж�����ͷ�����Ƿ����Range�ֶ�
			{
				//�����һ���ֿ鴫�䣬��Ҫ֪���ֿ������Ƕ���
				std::string range_str = req.get_header_value("Range");//��ȡ������bytes=start-end
				httplib::Ranges ranges;
				//ranges��ʵ��һ��vector<Range> ,Range��һ����ֵ��std::pair<start,end>
				httplib::detail::parse_range_header(range_str, ranges);//�����ͻ��˵�Range����
				int64_t range_start = ranges[0].first;//pari�е�first����range��startλ��
				int64_t range_end = ranges[0].second;//pair�е�second����range��endλ��
				int64_t range_len = range_end - range_start + 1;//�����range����ĳ���
				std::cout << realpath << "range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
			}
			else{
				//û��Rangeͷ��������һ���������ļ�����
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
			return;
		}
		else//��������HEAD�����---�ͻ���ֻҪͷ������Ҫ����
		{
			int64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//������Ӧͷ����Ϣ
			rsp.status = 200;
		}	
	} 
private:
	httplib::Server _srv;
};