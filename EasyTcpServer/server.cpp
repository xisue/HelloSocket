﻿#define WIN32_LEAN_AND_MEAN //windows.h和winsock2中重复包含了一些头文件，避免重复定义
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include <iostream>
using namespace std;

//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

struct DataPackage
{
	int age;
	char name[32];
};
int main()
{
	//启动windows socket2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	//建立一个套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//绑定端口和ip地址
	sockaddr_in _sin;
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;// inet_addr("192.168.1.77");
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
	{
		cout<<"server bind error...."<<endl;
	}
	else
	{
		cout<<"server bind success...."<<endl;
	}

	//设置最大监听数
	if (SOCKET_ERROR == listen(_sock, 128))
	{
		cout<<"server listen error...."<<endl;
	}
	else
	{
		cout<<"server listen success...."<<endl;
	}

	//阻塞等待客户端连接
	sockaddr_in clientAddr;
	int nAddrLen = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		cout<<"accept error,got a invalid client socket..."<<endl;
	}
	//输出客户端信息
	char clientIP[1024];
	printf("new connection:IP = %s, PORT = %d\n", \
		inet_ntop(AF_INET, (void*)&clientAddr.sin_addr, clientIP, sizeof(clientIP)), \
		ntohs(clientAddr.sin_port));

	while (true)
	{
		
		//接收客户端信息
		char _recvBuf[1024];
		int nLen = recv(_cSock, _recvBuf, 1024, 0);
		if (nLen<=0)
		{
			cout<<"客户端已退出,任务结束."<<endl;
			break;
		}
		//处理客户端请求
		if (0 == strcmp(_recvBuf, "getInfo"))
		{
			DataPackage info = { 24,"sue" };
			send(_cSock, (const char*)&info, sizeof(DataPackage), 0);
		}
		else
		{
			char msgBuf[] = "?????";
			send(_cSock, msgBuf, sizeof(msgBuf) + 1, 0);
		}
	}
	//关闭套接字
	closesocket(_cSock);
	closesocket(_sock);

	//清除windows socket2.x环境
	WSACleanup();
	getchar();
	return 0;
}