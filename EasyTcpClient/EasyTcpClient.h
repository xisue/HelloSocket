#ifndef _EASYTCPCLIENT_H_
#define _EASYTCPCLIENT_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN //windows.h和winsock2中重复包含了一些头文件，避免重复定义
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

typedef int SOCKET;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include<iostream>
#include "MsgHeader.h"
using namespace std;

class EasyTcpClient
{
public:
	SOCKET _sock;
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpClient()
	{
		Close();
	}
	//初始化socket
	void InitSocket()
	{
#ifdef _WIN32
		//启动windows socket2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			cout << "关闭之前的连接..." << endl;
			Close();
		}
		//建立一个socket
		_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (INVALID_SOCKET == _sock)
		{
			cout << "客户端无效的套接字..." << endl;
		}
		else
		{
			cout << "客户端创建套接字<socket:"<< _sock <<">..."<< endl;
		}
	}

	//连接服务器
	int Connect(const char* ip,unsigned short port)
	{
		sockaddr_in _sin;
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		inet_pton(AF_INET, ip, &_sin.sin_addr);
		if (INVALID_SOCKET == _sock)
			InitSocket();
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			cout << "客户端连接错误..." << endl;
		}
		else
		{
			cout << "客户端<socket:"<< _sock<<">连接"<<ip<<"成功..." << endl;
		}
		return ret;
	}

	//关闭socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
#ifdef _WIN32	
			//关闭socket
			closesocket(_sock);
			//清除windows socket2.x环境
			WSACleanup();
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}

	}
	//是否运行中
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}
	//处理网络消息，select监听服务端发来的消息，接收消息显示登录结果等消息
	bool onRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 0,0 };
			int ret = select(_sock + 1, &fdReads, NULL, NULL,&t);
			if (ret < 0)
			{
				cout << "select 任务结束1" << endl;
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);
				if (-1 == RecvData(_sock))
				{
					cout << "select 任务结束2" << endl;
					Close();
					return false;
				}
			}
		}
		return true;
	}

	//接收数据，处理粘包，拆分包
	int  RecvData(SOCKET _cSock)
	{
		//接收客户端信息
		char szRecv[1024];//缓冲区
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		if (nLen <= 0)
		{
			cout << "与服务器断开连接,任务结束." << endl;
			return -1;
		}
		DataHeader* header;
		header = (DataHeader*)szRecv;
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		
		onProcessMsg(header); //header和szRecv指向同一位置
		return 0;
	}

	//处理接收到的消息
	void onProcessMsg(DataHeader* header)
	{
		//根据消息头处理不同种类的信息
		switch (header->cmd)
		{
		case CMD_LOG_RESULT:
		{
			LogResult* logResult = (LogResult*)header;
			cout << "<socket:"<<_sock<<">收到服务端消息: 登录LoginResult," << " 数据长度： " << logResult->dataLength << endl;
			break;
		}
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult* logoutResult = (LogoutResult*)header;
			cout << "<socket:" << _sock << ">收到服务端消息: 登出LogoutResult," << " 数据长度： " << logoutResult->dataLength << endl;
			break;
		}
		case CMD_NEW_USER_JOIN:
		{
			NewUserJion* newUser = (NewUserJion*)header;
			cout << "<socket:" << _sock << ">收到服务端消息: 新用户加入NewUser<Socket: " << newUser->sock << ">, 数据长度： " << newUser->dataLength << endl;
			break;
		}
		}
	}
	//发送数据
	int SendData(DataHeader* header)
	{
		if (isRun()&&header)
		{
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
};

#endif
