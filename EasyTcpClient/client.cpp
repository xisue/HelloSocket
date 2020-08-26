#define WIN32_LEAN_AND_MEAN //windows.h和winsock2中重复包含了一些头文件，避免重复定义
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include <iostream>
using namespace std;


//#pragma comment(lib,"ws2_32.lib") //不利于跨平台，建议在属性里面修改，添加依赖项

enum CMD
{
	CMD_LOG,
	CMD_LOG_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
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
	}
	int result=0;
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
	}
	int result=0;
};
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
	
	while (true)
	{
		char cmdBuf[1024];
		cin>>cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			cout << "客户端退出" << endl;
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Log log;
			strcpy(log.username,"sue");
			strcpy(log.password, "memory");
			//向服务器发送数据
			send(_sock, (const char*)&log, sizeof(Log), 0);
			//接收服务器返回的数据
			LogResult logRet;
			recv(_sock, (char*)&logRet, sizeof(logRet), 0);
			cout << "Login Result: " << logRet.result << endl;
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.username,"sue");
			//向服务器发送数据
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
			//接收服务器返回的数据
			LogoutResult logoutRet;
			recv(_sock, (char*)&logoutRet, sizeof(logoutRet), 0);
			cout << "Login out Result: " << logoutRet.result << endl;
		}
		else
		{
			cout << "不支持的命令，请重新输入！" << endl;
		}
	}
	
	//关闭socket
	closesocket(_sock);
	//清除windows socket2.x环境
	WSACleanup();
	getchar();
	return 0;
}