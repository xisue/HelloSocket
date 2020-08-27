#include"EasyTcpServer.h"
#include <iostream>
#include<vector>
using namespace std;

//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

int main()
{
	EasyTcpServer server;
	//server.InitSocket();
	server.Bind(NULL, 4567);
	server.Listen(128);
	while (server.isRun())
	{
		server.onRun();
		
	}
	server.Close();
	cout << "服务端退出" << endl;
	getchar();
	return 0;
}
