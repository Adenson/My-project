#define _CRT_SECURE_NO_WARNINGS 1
#include"util.hpp"
#include"httplib.h"
void helloworld(const httplib::Request& req, httplib::Response& rsp)
{

	rsp.set_content("<html><h1>helloworld</h1></htm>", "text/html");
	rsp.status = 200;
}
int main()
{
	httplib::Server srv;
	srv.Get("/", helloworld);
	srv.listen("0.0.0.0", 9000);
	system("pause");
	return 0;
}