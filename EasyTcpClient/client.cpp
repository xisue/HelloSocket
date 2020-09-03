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
const int cCount = 50;//客户端数量
const int tCount = 4;//发送线程数量

EasyTcpClient* clients[cCount];

void sendThread(int id)
{
	int c = cCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;
	for (int n = begin; n < end; n++)
	{
		if (!bRun)
		{
			return ;
		}
		clients[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		if (!bRun)
		{
			return ;
		}
		clients[n]->Connect("127.0.0.1", 4567);
		cout << "connect: " << n << endl;
	}


	Log log;
	strcpy(log.username, "test");
	strcpy(log.password, "test_password123");

	while (bRun)
	{
		for (int n = begin; n < end; n++)
		{
			//clients[n].onRun();
			clients[n]->SendData(&log);
			cout << "client send: name:" << log.username << " password:" << log.password << endl;
		}

	}
	for (int n = begin; n < end; n++)
	{
		//clients[n].onRun();
		clients[n]->Close();
	}
}
int main()
{
	//启动UI线程
	thread t1(cmdThread);
	t1.detach();

	//启动发送线程
	for (int n = 0; n < cCount; n++)
	{
		thread t1(sendThread,n+1);
		t1.detach();
	}

	while (bRun)
	{
		Sleep(100);
	}
	
	cout << "已退出" << endl;
	return 0;
}
