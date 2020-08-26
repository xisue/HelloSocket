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

	//建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		cout << "client socket error..." << endl;
	}
	else
	{
		cout << "client socket success..." << endl;
	}
	//连接到服务器
	sockaddr_in _sin;
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	inet_pton(AF_INET,"192.168.1.77",&_sin.sin_addr);
	int ret=connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		cout<<"client connect error..."<<endl;
	}
	else
	{
		cout << "client connect success..."<<endl;
	}
	//向服务器发送命令
	char cmdBuf[1024];
	while (true)
	{
		cin>>cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			cout << "客户端退出" << endl;
			break;
		}
		else
		{
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}
		//接收服务器信息
		char recvBuf[1024];
		int nlen = recv(_sock, recvBuf, 1024, 0);
		if (nlen > 0)
		{
			cout<<"服务端响应: "<< recvBuf<<endl;
		}
	}
	
	//关闭socket
	closesocket(_sock);
	//清除windows socket2.x环境
	WSACleanup();
	getchar();
	return 0;
}