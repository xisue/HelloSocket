#include"EasyTcpServer.h"
#include <iostream>
#include<vector>
#include<thread>
using namespace std;

//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

bool bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[1024];
		cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			bRun = false;
			cout << "服务端线程退出" << endl;
			break;
		}
	}
	return;
}
int main()
{
	EasyTcpServer server;
	//server.InitSocket();
	server.Bind(NULL, 4567);
	server.Listen(128);
	//启动线程
	thread t1(cmdThread);
	t1.detach();
	while (bRun)
	{
		server.onRun();
		
	}
	server.Close();
	cout << "服务端退出" << endl;
	getchar();
	return 0;
}
