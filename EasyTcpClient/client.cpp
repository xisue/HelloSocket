#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include<thread>
using namespace std;

#include "EasyTcpClient.h"
//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

bool bRun = true;
void cmdThread(EasyTcpClient* client)
{
	while (true)
	{
		char cmdBuf[1024];
		cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			bRun = false;
			cout << "客户端<SOCKET: "<< client->_sock<<"> 线程退出" << endl;
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Log log;
			strcpy(log.username, "sue");
			strcpy(log.password, "memory");
			//向服务器发送数据
			client->SendData(&log);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.username, "sue");
			//向服务器发送数据
			client->SendData(&logout);
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
	EasyTcpClient client1;
	//client.InitSocket();
	client1.Connect("192.168.1.77", 4567);

	//启动线程
	thread t1(cmdThread ,&client1);
	t1.detach();
	while (client1.isRun())
	{
		client1.onRun();
	}
	client1.Close();
	cout << "已退出" << endl;
	getchar();
	return 0;
}
