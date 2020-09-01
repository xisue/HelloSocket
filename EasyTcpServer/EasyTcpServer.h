#ifndef _EASYTCPSERVER_H_
#define _EASYTCPSERVER_H_

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
	#define FD_SETSIZE 1024
	#define WIN32_LEAN_AND_MEAN //windows.h和winsock2中重复包含了一些头文件，避免重复定义
	#include<Windows.h>
	#include<WinSock2.h>
	#include<WS2tcpip.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h>

	typedef int SOCKET;
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif
#include "MsgHeader.h"
#include"CELLTimestamp.h"
#include<iostream>
#include<vector>
using namespace std;
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 
#endif
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char* msgBuf()
	{
		return _szMsgBuf;
	}
	int getLast()
	{
		return _lastPos;
	}
	void setLast(int pos)
	{
		_lastPos = pos;
	}
private:
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	int _lastPos = 0;//消息缓冲区尾部
};
class EasyTcpServer
{
private:
	SOCKET _sock;
	vector<ClientSocket*> _clients;
	CELLTimestamp _tTime;
	int _recvCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
	}
	virtual ~EasyTcpServer()
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
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			cout << "客户端无效的套接字..." << endl;
		}
		else
		{
			cout << "客户端创建套接字<socket:" << _sock << ">..." << endl;
		}

	}
	//绑定端口号
	int Bind(const char* ip,unsigned short port)
	{
		if (INVALID_SOCKET==_sock)
		{
			InitSocket();
		}
		//绑定端口和ip地址
		sockaddr_in _sin;
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		if (ip)
		{
#ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
			_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		}
		else
		{
#ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;// inet_addr("192.168.1.77");
#else
			_sin.sin_addr.s_addr = INADDR_ANY;
#endif
		}

		if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
		{
			cout << "服务端绑定端口"<<port<<"错误...." << endl;
			return -1;
		}
		else
		{
			cout << "服务端绑定端口" << port << "成功...." << endl;
		}
		return 0;
	}
	//监听端口号
	int Listen(int n)
	{
		//设置最大监听数
		int ret = listen(_sock, n);
		if (SOCKET_ERROR ==ret )
		{
			cout << "<socket:"<<_sock<<">服务器监听失败...." << endl;
		}
		else
		{
			cout << "<socket:" << _sock << "服务器监听成功...." << endl;
		}
		return ret;
	}
	//接受客户端连接
	SOCKET Accept()
	{
		//阻塞等待客户端连接
		sockaddr_in clientAddr;
		int nAddrLen = sizeof(clientAddr);
		SOCKET cSock = INVALID_SOCKET;

		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
		if (INVALID_SOCKET == cSock)
		{
			cout << "<socket:"<<_sock<<">连接错误的套接字..." << endl;
		}
		else
		{
			NewUserJion newuser;
			newuser.sock = cSock;
			SendDataToAll(&newuser);
			//输出客户端信息
			char clientIP[1024];
			printf("<Socket:%d>新客户端<%d>连接:IP = %s, PORT = %d\n",_sock,_clients.size(),\
				inet_ntop(AF_INET, (void*)&clientAddr.sin_addr, clientIP, sizeof(clientIP)), \
				ntohs(clientAddr.sin_port));
			_clients.push_back(new ClientSocket(cSock));
		}
		return cSock;
	}
	//关闭socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
#ifdef _WIN32	
			for (int n=(int)_clients.size()-1;n>=0;n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			//关闭socket
			closesocket(_sock);
			//清除windows socket2.x环境
			WSACleanup();
#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}

	}
	//是否在工作中
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}
	//处理消息
	int count = 0;
	bool onRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExp;

			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);
			SOCKET maxSock = _sock;
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (_clients[n]->sockfd() > maxSock)
					maxSock = _clients[n]->sockfd();
			}
			//nfds为一个整数值，指fd_set集合中所有描述符的范围（最大值+1），window可以直接传0
			timeval t;
			t.tv_sec = 0;
			t.tv_usec = 0;
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);//NULL阻塞监听，0非阻塞监听，轮询
			if (ret < 0)
			{
				cout << "<socket:"<<_sock<<"> select 任务结束" << endl;
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
			}

			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						auto iter = _clients.begin() + n;
						if (iter != _clients.end())
						{
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}

			}
			return true;
		}
		return false;
	}

	char _szRecv[RECV_BUFF_SIZE];//缓冲区
	//接受数据，处理粘包，拆分包
	int  RecvData(ClientSocket* pClient)
	{
		//接收客户端信息
		int nLen = (int)recv(pClient->sockfd(), _szRecv, sizeof(DataHeader), 0);
		if (nLen <= 0)
		{
			cout << "客户端<SOCKET: " << pClient->sockfd() << ">已退出,服务端任务结束." << endl;
			return -1;
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLast(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLast(pClient->getLast() + nLen);
		//判断消息缓冲区的数据长度大于消息头长度
		while (pClient->getLast() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			//消息缓冲区剩余数据长度大于消息长度
			if (pClient->getLast() >= header->dataLength)
			{
				//消息缓冲区剩余未处理的数据长度
				int nSize = pClient->getLast() - header->dataLength;
				//处理网络消息
				onProcessMsg(header,pClient->sockfd());
				//将消息缓冲区剩余未处理的数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//消息缓冲区的数据尾部位置前移
				pClient->setLast(nSize);
			}
			else
			{
				//剩余数据不够一条完整的消息
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	virtual	void onProcessMsg(DataHeader* header,SOCKET cSock)
	{
		_recvCount++;
		auto t1 = _tTime.getElapsedSecond();
		if (t1>=1.0)
		{
			printf("time<%lf>,socket<%d>,recvCount<%d>\n", t1, cSock, _recvCount);
			_recvCount = 0;
			_tTime.update();
		}
		//根据消息头处理不同种类的信息
		switch (header->cmd)
		{
		case CMD_LOG:
		{
			Log* log = (Log*)header;
			//cout << "收到<SOCKET: " << cSock << ">命令: Login , " << " 数据长度： " << log->dataLength << endl;
			//cout << "姓名： " << log->username << " 密码： " << log->password << endl;
			//LogResult ret;
			//send(cSock, (char*)&ret, sizeof(LogResult), 0);
			break;
		}
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//cout << "收到<SOCKET: " << cSock << ">命令: Loginout , " << " 数据长度： " << logout->dataLength << endl;
			//cout << "姓名： " << logout->username << endl;
			//LogoutResult ret;
			//send(cSock, (char*)&ret, sizeof(LogoutResult), 0);
			break;
		}
		default:
		{
			
			cout << "<socket:" << _sock << ">收到未定义消息, 数据长度： " << header->dataLength << endl;
			//DataHeader ret;
			//SendData(&ret, cSock);
			break;
		}
		}
	}
	//发送数据给单个客户端
	int SendData(DataHeader* header, SOCKET cSock)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	//发送数据给所有客户端
	int SendDataToAll(DataHeader* header)
	{
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			SendData(header, _clients[n]->sockfd());
		}
		return 0;
	}

};


#endif