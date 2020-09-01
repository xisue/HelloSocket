#ifndef _MSGHEADER_H_
#define _MSGHEADER_H_
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
public:
	DataHeader()
	{
		dataLength = sizeof(DataHeader);
		cmd = CMD_ERROR;
	}
	
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
	char password[32];
	char data[956];
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
	char data[1016];
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
	int sock;
};
#endif