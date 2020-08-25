#define WIN32_LEAN_AND_MEAN //windows.h和winsock2中重复包含了一些头文件，避免重复定义
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include <iostream>
using namespace std;

//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项
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
		printf("server bind error....\n");
	}
	else
	{
		printf("server bind success....\n");
	}

	//设置最大监听数
	if (SOCKET_ERROR == listen(_sock, 128))
	{
		printf("server listen error....\n");
	}
	else
	{
		printf("server listen success....\n");
	}

	//阻塞等待客户端连接
	sockaddr_in clientAddr;
	int nAddrLen = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;
	char msgBuf[] = "Hello, I am Server.";
	while (true)
	{
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
		if (INVALID_SOCKET == _cSock)
		{
			printf("accept error,got a invalid client socket...\n");
		}
		char clientIP[1024];
		printf("new connection:IP = %s, PORT = %d\n", inet_ntop(AF_INET,(void*)&clientAddr.sin_addr,clientIP,sizeof(clientIP)),ntohs(clientAddr.sin_port));
		//向客户端发送一条数据
		send(_cSock, msgBuf, sizeof(msgBuf) + 1, 0);
	}

	//关闭套接字
	closesocket(_sock);

	//清除windows socket2.x环境
	WSACleanup();
	return 0;
}