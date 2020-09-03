#ifndef _EASYTCPSERVER_H_
#define _EASYTCPSERVER_H_

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
	#define FD_SETSIZE 4024
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



#include<iostream>
#include<vector>
#include<thread>
#include<mutex>
#include<atomic>

#include "MsgHeader.h"
#include"CELLTimestamp.h"

using namespace std;

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 
#endif

//客户端数据类型
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
	//发送数据
	int SendData(DataHeader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
private:
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 5];
	//消息缓冲区尾部
	int _lastPos = 0;
};

//网络事件接口
class INetEvent
{
public:
	//客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	//客户端离开事件
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	//接收包计数
	virtual void OnNetMsg(ClientSocket* pClient,DataHeader* header) = 0;
};

class CellServer
{
public:
	CellServer(SOCKET sock=INVALID_SOCKET)
	{
		_sock = sock;
		_pNetEvent = nullptr;
	}
	~CellServer()
	{
		Close();
		_sock = INVALID_SOCKET;
	}
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;

	}

	void Start()
	{

		_thread = thread(&CellServer::onRun, this);

	}

	//关闭socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
#ifdef _WIN32	
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			//关闭socket
			closesocket(_sock);
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

	void addClient(ClientSocket* pClient)
	{
		//_mutex.lock();
		lock_guard<mutex>lock(_mutex);
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}
	
	//是否在工作中
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	bool onRun()
	{
		while (isRun())
		{
			//从缓冲队列中取出客户数据
			if (_clientsBuff.size()>0)
			{
				lock_guard<mutex>lock(_mutex);
				for (auto pClient:_clientsBuff)
				{
					_clients.push_back(pClient);
					
				}
				_clientsBuff.clear();
			}
			if (_clients.empty())
			{
				chrono::milliseconds t(1);
				this_thread::sleep_for(t);
				continue;
			}
			fd_set fdRead;

			FD_ZERO(&fdRead);

			SOCKET maxSock = _clients[0]->sockfd();
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (_clients[n]->sockfd() > maxSock)
					maxSock = _clients[n]->sockfd();
			}
			//nfds为一个整数值，指fd_set集合中所有描述符的范围（最大值+1），window可以直接传0
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);//NULL阻塞监听，0非阻塞监听，轮询
			if (ret < 0)
			{
				cout << "<socket:" << _sock << "> select 任务结束" << endl;
				Close();
				return false;
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
							if (_pNetEvent)
								_pNetEvent->OnNetLeave(_clients[n]);
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}

			}
		}
		return false;
	}

	//接受数据，处理粘包，拆分包
	char _szRecv[RECV_BUFF_SIZE];//缓冲区
	int  RecvData(ClientSocket* pClient)
	{
		//接收客户端信息
		int nLen = (int)recv(pClient->sockfd(), _szRecv, sizeof(DataHeader), 0);
		if (nLen <= 0)
		{
			//cout << "客户端<SOCKET: " << pClient->sockfd() << ">已退出,服务端任务结束." << endl;
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

				onProcessMsg(header, pClient);
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
	virtual	void onProcessMsg(DataHeader* header, ClientSocket* pClient)
	{
		_pNetEvent->OnNetMsg(pClient, header);
	}

private:
	SOCKET _sock;
	//正式客户列表
	vector<ClientSocket*> _clients;
	//缓冲客户列表
	vector<ClientSocket*> _clientsBuff;
	//缓冲队列的锁
	mutex  _mutex;
	thread _thread;
	//网络事件对象
	INetEvent* _pNetEvent;
};

class EasyTcpServer:public INetEvent
{

public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
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
			//将客户端分配给处理客户端数量最少的服务器线程
			addClientToServer(new ClientSocket(cSock));
		}
		return cSock;
	}
	
	void addClientToServer(ClientSocket* pClient)
	{
		//查找客户数量最小的cellServer消息处理对象
		auto pMinServer = _cellServer[0];
		for (auto pCellServer:_cellServer)
		{
			if (pMinServer->getClientCount()>pCellServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
		OnNetJoin(pClient);

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

	void Start(int nClientServer)
	{
		for (int n=0;n< nClientServer;n++)
		{
			auto ser = new CellServer(_sock);
			_cellServer.push_back(ser);
			//注册网络事件
			ser->setEventObj(this);
			//启动消息处理线程
			ser->Start();
		}
	}
	
	//是否在工作中
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}
	
	//处理网络消息
	bool onRun()
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			//清理集合
			FD_ZERO(&fdRead);
			//将描述符加入集合
			FD_SET(_sock, &fdRead);
			
			//nfds为一个整数值，指fd_set集合中所有描述符的范围（最大值+1），window可以直接传0
			timeval t;
			t.tv_sec = 0;
			t.tv_usec = 0;
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);//NULL阻塞监听，0非阻塞监听，轮询
			if (ret < 0)
			{
				cout << "<socket:" << _sock << "> select 任务结束" << endl;
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}
	
	//计数并输出每秒收到的网络消息
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1>=1.0)
		{
			printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recvCount<%d>\n",_cellServer.size(), t1, _sock, (int)_clientCount, (int)(_recvCount/t1));
			_recvCount = 0;
			_tTime.update();
		}
	}

	//只会被一个线程触发，多个线程触发不安全
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}
	//只会被一个线程触发，多个线程触发不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}
	//只会被一个线程触发，多个线程触发不安全
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
	}

	
private:
	SOCKET _sock;
	//消息处理对象，内部会创建线程
	vector<CellServer*> _cellServer;
	//每秒消息计时
	CELLTimestamp _tTime;
	protected:
	//收到消息包个数
	atomic_int  _recvCount;
	//客户端计数
	atomic_int  _clientCount;
};


#endif