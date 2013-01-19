#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

#ifdef UNICODE
#define TSTRING std::wstring
#else
#define TSTRING std::string
#endif

#define COMM_UNENCRYPTED 0x1
#define COMM_COMPRESSED 0x2

#define COMM_TIMEOUT 1000


//#define TEST_FLAG(x, flag) ((x) & flag == flag)

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

class Waitable
{
public:
	bool Running;
	Waitable() : Running(false) { }
};

typedef int (Waitable::*WaitHandler)();

void EncodeString(const TCHAR* str, BYTE* res);
void Compress(const void* data, const unsigned int size, BYTE* comped, unsigned int* len);
void Decompress(const void* data, const unsigned int size, BYTE* decomped, unsigned int* len);

#endif