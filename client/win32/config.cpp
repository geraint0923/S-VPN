//
#include "config.h"
#include "md5.h"

void Config::UseDebugConfig()
{
	sinfo.ServerName = "www.huxx.me";
	sinfo.ServerPort = 33333;
	uinfo.UserName = "a";
	MD5Fast("a", 1, uinfo.MD5);
}