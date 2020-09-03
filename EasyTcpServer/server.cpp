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
class MyServer:public EasyTcpServer
{
public:
	MyServer()
	{

	}
	~MyServer()
	{

	}
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
		printf("client<%d> join\n", pClient->sockfd());
	}
	//只会被一个线程触发，多个线程触发不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
		printf("client<%d> leave\n", pClient->sockfd());
	}
	//只会被一个线程触发，多个线程触发不安全
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
		//根据消息头处理不同种类的信息
		switch (header->cmd)
		{
		case CMD_LOG:
		{

			Log* log = (Log*)header;
			LogResult ret;
			pClient->SendData(&ret);
			break;
		}
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			LogoutResult ret;
			pClient->SendData(&ret);
			break;
		}
		default:
		{

			cout << "<socket:" << pClient->sockfd() << ">收到未定义消息, 数据长度： " << header->dataLength << endl;
			break;
		}
		}
	}
private:

};
int main()
{
	MyServer server;
	//server.InitSocket();
	server.Bind(NULL, 4567);
	server.Listen(128);
	server.Start(4);
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
