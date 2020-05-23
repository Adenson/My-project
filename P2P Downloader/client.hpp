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
	uint32_t _ip_addr;//要配对主机的IP地址
	bool _pair_ret;//用于存放配对结果，配对成功为true,配对失败为false 
};
class Client
{
public:
	bool Start()
	{
		//客户端程序需要循环运行，因为下载文件不是只下载一次
		while (1)
		{
			GetOnlineHost();
		}
		return true;
	}
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
		//没必要每次都需要发起主机配对请求，所以在配对之前需要进行检查
		char ch = 'Y';
		if (!_online_host.empty())
		{
			std::cout << "是否重新匹配在线主机(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "开始主机匹配...\n";
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
				for (int j = 1; j < (int32_t)max_host; j++)
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
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "正在主机匹配中，请稍等一会儿......\n";
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

		return true;
	} 

	//获取文件列表
	bool GetShareList(const std::string& host_ip)
	{
		//1.先发送请求
		//2.得到响应之后，解析正文（文件名称）
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
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
		RangeDownLoadFile(host_ip, filename);
		return true;
	}

	//下载文件（若文件一次性进行下的话如果遇到大文件会比较危险）
	bool DownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.向服务端发送文件下载请求“/filename”
		//2.得到响应结果，响应结果中的body正文就是文件数据
		//3.创建文件，将文件数据写入文件中，关闭文件

		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "下载文件，获取响应信息失败\n";
			return false;
		}
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "已经成功下载文件！！！" << std::endl;
		return true;
	}

	//分块下载
	bool RangeDownLoadFile(const std::string& host_ip, const std::string& filename)
	{
		//1.发送HEAD请求，通过响应中的Content-Length获取文件大小
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str()); 
		if (rsp == NULL || rsp->status != 200)
		{
			std::cout << "获取文件大小信息失败\n";
			return false;
		}
		//通过HTTP头部字段名获取值
		std::string clen = rsp->get_header_value("Content-Length");
		int64_t filesize = StringUtil::Str2Dig(clen);
		//2.根据文件大小进行分块
		  //若文件大小 < 块大小，则直接下载文件
		if (filesize < MAX_RANGE)
		{
			return DownLoadFile(host_ip, filename);
		}
		  //若文件大小不能整除块大小，则分块个数为文件大小除以分块大小+1
		  //若文件大小刚好整除块大小，得到响应之后写入文件的指定位置
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

			//3.逐一请求分块数据，得到响应之后写入文件的指定位置
			httplib::Headers header; 
			header.insert(httplib::make_range_header({ { range_start, range_end } })); //设置一个range区间
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206)
			{
				std::cout << "区间下载文件失败\n";
				return false;
			}
			FileUtil::Write(filename, rsp->body, range_start);
		}
		std::cout << "文件下载成功\n";
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
		//在httplib中，Get接口不但针对客户端的GET请求方法，也针对客户端的HEAD请求方法，只需要在Downlowd函数中进行判断就可以

		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	//只能设置成为static，因为httplib中的函数没有this指针
	static void HostPair(const httplib::Request& req, httplib::Response& rsp)
	{
		rsp.status = 200;
		return;
	}

	//获取文件列表（在主机上设置一个共享目录，凡是这个目录下的文件都是要共享给别人的）
	static void ShareList(const httplib::Request& req, httplib::Response& rsp)
	{
		//查看目录是否存在，若目录不存在，则创建这个目录
		if (!boost::filesystem::exists(SHARED_PATH))
		{
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);//实例化目录迭代器
		boost::filesystem::directory_iterator end;//实例化目录迭代器的末尾
		for (; begin != end; ++begin)
		{
			//bigin->status() 目录中当前文件的状态信息
			//boost::filesystem::is_directory()判断当前文件是否是一个目录
			if (boost::filesystem::is_directory(begin->status())){
				//此处我只考虑了获取普通文件名称，没有多层级目录的操作
				continue;
			}
			else{
				std::string name = begin->path().filename().string();//将返回的文件设置成为不带有路径的
				rsp.body += name + "\r\n";
			}
		}
		rsp.status = 200;
		return;
	}

	static void Download(const httplib::Request& req, httplib::Response& rsp)
	{
		//req.path(客户端请求的资源路径,当初客户端前面加的有download，比如：/download/filename.txt）
		boost::filesystem::path req_path(req.path);
		//此处只获取文件名称:filename.txt
		std::string name = req_path.filename().string();
		//实际文件的路径应该是在共享的目录下
		std::string realpath = SHARED_PATH + name;
		//判断文件是否存在
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;
			return;
		}

		if (req.method == "GET")//针对客户端GET请求方法
		{ 
			//GET请求，就是直接下载完整文件，但是增加了分块传输的功能后，就需要判断是否是分块传输的依据，就是这次请求中是否有Range头部字段
			if (req.has_header("Range"))//判断请求头部中是否包含Range字段
			{
				//这个是一个分块传输，需要知道分块区间是多少
				std::string range_str = req.get_header_value("Range");//获取到的是bytes=start-end
				httplib::Ranges ranges;
				//ranges其实是一个vector<Range> ,Range是一个键值对std::pair<start,end>
				httplib::detail::parse_range_header(range_str, ranges);//解析客户端的Range数据
				int64_t range_start = ranges[0].first;//pari中的first就是range的start位置
				int64_t range_end = ranges[0].second;//pair中的second就是range的end位置
				int64_t range_len = range_end - range_start + 1;//计算出range区间的长度
				std::cout << realpath << "range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
			}
			else{
				//没有Range头部，则是一个完整的文件下载
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
			return;
		}
		else//这个是针对HEAD请求的---客户端只要头部不需要正文
		{
			int64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//设置响应头部信息
			rsp.status = 200;
		}	
	} 
private:
	httplib::Server _srv;
};