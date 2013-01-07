//
#include <WinSock2.h>
#include "config.h"
#include "md5.h"

void Config::UseDebugConfig()
{
	sinfo.ServerName = "36.54.3.49";//"www.huxx.me";
	sinfo.ServerPort = 33333;
	uinfo.UserName = "a";
	MD5Fast("a", 1, uinfo.MD5);

	tunaddr = inet_addr("192.168.3.6");
}

void Config::UseCommandLineConfig(char** argv)
{
	sinfo.ServerName = std::string(argv[2]);
	sinfo.ServerPort = atoi(argv[3]);
	uinfo.UserName = "a";
	MD5Fast("a", 1, uinfo.MD5);

	tunaddr = inet_addr(argv[1]);
}