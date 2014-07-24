#ifndef __COMM_H__
#define __COMM_H__

#include <string>
#include <WinSock2.h>
#include "crypt.h"
#include "config.h"

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

enum CommStatus
{
	Init,
	Connected,
	Disconnected,
	Error
};

class CommClient
{
public:
	static int InitEnvironment();
	static int CleanEnvironment();

	CommClient();
	~CommClient();
	int Connect(const ServerInfo& sinfo, const UserInfo& uinfo);
	int Disconnect();
	int SendEncrypt(const void* buf, unsigned long size);

	HANDLE InitRecv();
	int BeginRecvDecrypt();
	int EndRecvDecrypt(void* buf, unsigned long& size);

	HANDLE InitRecv2();
	int BeginRecvDecrypt2();
	int EndRecvDecrypt2(void* buf, unsigned long& size);
public:
	CommStatus Status;
	std::string UserName;
	unsigned char PasswordMD5[16];
	CodeTable CodeBook;
	SOCKET ClientSock;
	SOCKET ClientSock2;

	sockaddr ServerAddr;
	sockaddr ServerAddr2;
	int ServerAddrLen;

	WSAOVERLAPPED overlapRead;
	int readState;
	HANDLE readEvent;
	WSAOVERLAPPED overlapRead2;
	int readState2;
	HANDLE readEvent2;

	char innerBuf[10000];
	DWORD innerLength;

	char innerBuf2[10000];
	DWORD innerLength2;
};



#endif