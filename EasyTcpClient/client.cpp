#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include<thread>
using namespace std;

#include "EasyTcpClient.h"
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
			cout << "客户端线程退出" << endl;
			bRun = false;
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Log log;
			strcpy(log.username, "sue");
			strcpy(log.password, "memory");
			//向服务器发送数据
			//client->SendData(&log);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.username, "sue");
			//向服务器发送数据
			//client->SendData(&logout);
		}
		else
		{
			cout << "不支持的命令，请重新输入！" << endl;
		}
	}
	return;
}


int main()
{
	//EasyTcpClient client1;
	//client.InitSocket();
	//client1.Connect("192.168.1.77", 4567);
	const int count = 63;
	EasyTcpClient *clients[count];
	for (int n=0;n<count;n++)
	{
		clients[n] = new EasyTcpClient();
	}
	for (int n = 0; n < count; n++)
	{
		clients[n]->Connect("192.168.1.77", 4567);
	}
	//启动线程
	thread t1(cmdThread);
	t1.detach();

	Log login;
	strcpy(login.username, "test");
	strcpy(login.password, "test_password123");

	while (bRun)
	{
		for (int n = 0; n < count; n++)
		{
			//clients[n].onRun();
			clients[n]->SendData(&login);
		}
		
	}
	for (int n = 0; n < count; n++)
	{
		//clients[n].onRun();
		clients[n]->Close();
	}
	cout << "已退出" << endl;
	getchar();
	return 0;
}
