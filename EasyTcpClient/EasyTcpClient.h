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
private:
	SOCKET _sock;
	bool isConnect;
public:
	
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
		isConnect = false;
	}

	virtual ~EasyTcpClient()
	{
		isConnect = false;
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
			isConnect = true;
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
		isConnect = false;
	}
	//是否运行中
	bool isRun()
	{
		return INVALID_SOCKET != _sock&&isConnect;
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
	//接收缓冲区大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 
#endif
	//接收缓冲区
	char _szRecv[RECV_BUFF_SIZE];
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 5];
	int _lastPos = 0;
	//接收数据，处理粘包，拆分包
	int  RecvData(SOCKET _cSock)
	{
		//接收客户端信息
		int nLen = (int)recv(_cSock, _szRecv, RECV_BUFF_SIZE, 0);

		if (nLen <= 0)
		{
			cout << "与服务器断开连接,任务结束." << endl;
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(_szMsgBuf+_lastPos, _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		_lastPos += nLen;
		//判断消息缓冲区的数据长度大于消息头长度
		while(_lastPos>= sizeof(DataHeader))
		{
			DataHeader* header= (DataHeader*)_szMsgBuf;
			//消息缓冲区剩余数据长度大于消息长度
			if (_lastPos>=header->dataLength)
			{
				//消息缓冲区剩余未处理的数据长度
				int nSize = _lastPos - header->dataLength;
				//处理网络消息
				onProcessMsg(header);
				//将消息缓冲区剩余未处理的数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				_lastPos = nSize;
			}
			else
			{
				//剩余数据不够一条完整的消息
				break;
			}
		}

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
		case CMD_ERROR:
		{
			cout << "<socket:" << _sock << ">收到服务器消息ERROR, 数据长度： " << header->dataLength << endl;
			break;
		}
		default:
		{
			cout << "<socket:" << _sock << ">收到未定义消息, 数据长度： " << header->dataLength << endl;
			
		}
		}
	}
	//发送数据
	int SendData(DataHeader* header,int nLen)
	{
		int ret = SOCKET_ERROR;
		if (isRun()&&header)
		{
			ret = send(_sock, (const char*)header, nLen, 0);
			if (SOCKET_ERROR == ret)
				Close();
		}
		return ret;
	}
};

#endif
