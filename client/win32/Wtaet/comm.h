#ifndef __COMM_H__
#define __COMM_H__

#include <string>
#include <WinSock2.h>
#include "crypt.h"
#include "config.h"

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

class Client;

enum CommStatus
{
	Init,
	Connected,
	Disconnected,
	Error
};

#define COMM_READ_STATUS_NONE 0
#define COMM_READ_STATUS_PENDING 1
#define COMM_READ_STATUS_DONE 2

class CommClient : public Waitable
{
public:
	static int InitEnvironment();
	static int CleanEnvironment();

	CommClient();
	~CommClient();

	int Connect();//const TCHAR* ServerName, const TCHAR* ServerPort);
	socket_t Connect(const sockaddr* serverAddr, int flag);

	void InitRecv();
	int Send(const void* data, unsigned int length, int flag);
	int BeginRecv();
	int EndRecv(void* buf, unsigned int* size);

	int UpdateCodeBook(const TCHAR* userName, long long timeStamp);
	int UpdateServerAddress(const sockaddr* ServerAddr, const sockaddr* dns, int Options);
public:
	Client* Current;
	CodeTable* CodeBook;

	// Statistics info
	unsigned int sTotalPackagesSend;
	unsigned int sTotalPackagesRecv;
	unsigned long long sTotalBytesSend;
	unsigned long long sTotalBytesRecv;

	CommStatus Status;
	SOCKET ClientSock;

	sockaddr ServerAddr;
	int ServerAddrLen;

#ifdef WIN32
	WSAOVERLAPPED overlapRead;
	char readState;
	HANDLE readEvent;
#endif

	int PackageHandler(HANDLE hEvent);

	char innerBuf[PACKAGE_BUFFER_SIZE];
	DWORD innerLength;
};



#endif