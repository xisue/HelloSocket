#define _CRT_SECURE_NO_WARNINGS
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


#include <iostream>
#include<thread>
using namespace std;


//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

enum CMD
{
	CMD_LOG,
	CMD_LOG_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
struct DataHeader
{
	short dataLength;
	short cmd;
};
struct Log :public DataHeader
{
	Log() {
		dataLength = sizeof(Log);
		cmd = CMD_LOG;
	}
	char username[32];
	char password[128];
};
struct LogResult :public DataHeader
{
	LogResult()
	{
		dataLength = sizeof(LogResult);
		cmd = CMD_LOG_RESULT;
		result = 0;
	}
	int result;
};
struct Logout :public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char username[32];
};
struct LogoutResult :public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};
struct NewUserJion :public DataHeader
{
	NewUserJion()
	{
		dataLength = sizeof(NewUserJion);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	SOCKET sock;
};
int  processor(SOCKET _cSock)
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
	//处理客户端请求
	switch (header->cmd)
	{
	case CMD_LOG_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogResult* logResult = (LogResult*)szRecv;
		cout << "收到服务端消息: 登录 " << logResult->cmd << " 数据长度： " << logResult->dataLength << endl;
		break;
	}
	case CMD_LOGOUT_RESULT:
	{

		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult* logoutResult = (LogoutResult*)szRecv;
		cout << "收到服务端消息: 登出 " << logoutResult->cmd << " 数据长度： " << logoutResult->dataLength << endl;
		break;
	}
	case CMD_NEW_USER_JOIN:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJion* newUser = (NewUserJion*)szRecv;
		cout << "收到服务端消息: 新用户加入" << newUser->cmd << " 数据长度： " << newUser->dataLength << endl;
		cout << "新用户socket：" << newUser->sock << endl;
		break;
	}
	}
	return 0;
}bool bRun = true;
void cmdThread(SOCKET _sock)
{
	while (true)
	{
		char cmdBuf[1024];
		cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			bRun = false;
			cout << "客户端<SOCKET: "<<_sock<<"> 线程退出" << endl;
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Log log;
			strcpy(log.username, "sue");
			strcpy(log.password, "memory");
			//向服务器发送数据
			send(_sock, (const char*)&log, sizeof(Log), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.username, "sue");
			//向服务器发送数据
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
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
#ifdef _WIN32
	//启动windows socket2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	//建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		cout << "客户端无效的套接字..." << endl;
	}
	else
	{
		cout << "客户端创建套接字..." << endl;
	}
	//连接到服务器
	sockaddr_in _sin;
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	inet_pton(AF_INET,"192.168.1.77",&_sin.sin_addr);
	int ret=connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		cout<<"客户端连接错误..."<<endl;
	}
	else
	{
		cout << "客户端连接成功..."<<endl;
	}
	//启动线程
	thread t1(cmdThread,_sock);
	t1.detach();
	//向服务器发送命令
	while (bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1,0 };
		int ret=select(_sock+1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			cout << "select 任务结束1" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdReads))
		{
			FD_CLR(_sock, &fdReads);
			if (-1 == processor(_sock))
			{
				cout << "select 任务结束2" << endl;
				break;
			}
		}
	}
#ifdef _WIN32	
	//关闭socket
	closesocket(_sock);
	//清除windows socket2.x环境
	WSACleanup();
#else
	close(_sock);
#endif
	cout << "客户端已退出" << endl;
	getchar();
	getchar();
	return 0;
}
