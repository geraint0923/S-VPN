#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

#ifdef UNICODE
#define TSTRING std::wstring
#else
#define TSTRING std::string
#endif

class ServerInfo
{
public:
	std::string ServerName;
	unsigned short ServerPort;
};

class UserInfo
{
public:
	std::string UserName;
	unsigned char MD5[16];
};

class Config
{
public:
	ServerInfo sinfo;
	UserInfo uinfo;
	unsigned long tunaddr;
public:
	void UseDebugConfig();
	void UseCommandLineConfig(char** argv);
	//int ReadConfigFile();
};



#endif