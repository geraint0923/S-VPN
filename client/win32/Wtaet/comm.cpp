//
#include "comm.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdint.h>
#include "Client.h"
#include "common.h"

#pragma comment(lib, "ws2_32.lib")
//33333
// a 0
// www.huxx.me

CommClient::CommClient()
{
	Status = Init;
	uint8_t d;
}

CommClient::~CommClient()
{
	if (CodeBook)
		delete CodeBook;
}

int CommClient::Connect(const TCHAR* ServerName, const TCHAR* ServerPort)
{
	sockaddr_storage ds;
	// SND solve
	ADDRINFOT hints;
	ZeroMemory(&hints, sizeof(hints));
    hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	ADDRINFOT* addrInfo;
	GetAddrInfo(ServerName, ServerPort, &hints, &addrInfo);
	if (addrInfo == NULL)
	{
		printf("Error resolving server address.\n");
		return 1;
	}
	// ServerAddrLen = addrInfo->ai_addrlen;
	sockaddr addr = *addrInfo->ai_addr;
	FreeAddrInfo(addrInfo);

	return Connect(&addr, 0);
}

inline void OptimizeSocketParameter(SOCKET sock, int flag1, int flag2)
{
	int sendbuff = 1024*4;
//	socklen_t optlen = sizeof(sendbuff);
	int ree = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendbuff, sizeof(sendbuff));
//	sendbuff = 1024*128;
//	ree = setsockopt(ClientSock, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuff, optlen);
}

SOCKET CommClient::Connect(const sockaddr* serverAddr, int flag)
{
	ServerAddr = *serverAddr;
	SOCKET sock;
	if (flag == 0)
	{
		// create socket
		printf("Error. IPV4 or IPV6 NOT CLEAR. Line.%d\n", __LINE__);
		sock = WSASocket(
			(serverAddr->sa_family == 0) ? AF_INET6 : AF_INET,
			SOCK_DGRAM,
			(serverAddr->sa_family == 0) ? IPPROTO_IPV6 : IPPROTO_IP,
			NULL, 0, WSA_FLAG_OVERLAPPED);
		if (sock == INVALID_SOCKET)
			return INVALID_SOCKET;
		// optimize parameters
		OptimizeSocketParameter(sock, 0, 0);
		// connect
		// bind local address
		sockaddr localAddr;
		if (serverAddr->sa_family == 0) // ipv6
		{
			printf("Not implemetned.\n");
		}
		else
		{
			((sockaddr_in*)(&localAddr))->sin_addr.S_un.S_addr = INADDR_ANY;
			((sockaddr_in*)(&localAddr))->sin_family = AF_INET;
			((sockaddr_in*)(&localAddr))->sin_port = htons(0);
		}
		bind(ClientSock, (sockaddr*)&localAddr, sizeof(sockaddr));
		return sock;
	}
	else // tcp
	{
		printf("TCP not implemented.\n");
		return INVALID_SOCKET;
	}

	// debug
	unsigned char buf[1000];
	buf[0] = PACKAGE_UNENCRYPTED;
	strcpy(&buf[1], "hello");
	Send(buf, 7, 0);

	InitRecv();
	BeginRecv();
	Current->RegisterEvent(readEvent, this, (WaitHandler)CommClient::PackageHandler);
}

int CommClient::PackageHandler(HANDLE hEvent)
{
	unsigned char buf[10000];
	unsigned int size;
	this->EndRecv(buf, &size);
	printf("hello.\n");
}

int CommClient::UpdateServerAddress(const sockaddr* serverAddr, const sockaddr* dnsAddr, int options)
{
	// no difference
	if (memcmp(&ServerAddr, serverAddr, sizeof(sockaddr)) == 0)
		return 0;
	// reconnect
	// TODO: reconnect
	printf("Error. Reconnect not implemented.\n");
	// TODO: update dns
	return 0;
}

#define MAX_USERNAME_LENGTH 1000

int CommClient::UpdateCodeBook(const TCHAR* userName, long long timeStamp)
{
	if (CodeBook == NULL)
		CodeBook = new CodeTable;
	unsigned char strbuf[MAX_USERNAME_LENGTH];
	//EncodeString(userName, strbuf);
	strcpy((signed char*)strbuf, userName);
	BuildTable(CodeBook, strbuf, timeStamp);
	return 0;
}

struct CommPackage
{
	BYTE Flag;
	BYTE Load[0];
};

int CommClient::Send(const void* buf, unsigned int size, int flag)
{
	BYTE sbuf[PACKAGE_BUFFER_SIZE];
	CommPackage* pkgbuf = (CommPackage*)sbuf;
	pkgbuf->Flag = 0;
	unsigned int len;
	// compress
	if (flag & COMM_COMPRESSED)
	{
		Compress(buf, size, pkgbuf->Load, &len);
		pkgbuf->Flag |= COMM_COMPRESSED;
	}
	else
	{
		len = size;
		memcpy(pkgbuf->Load, buf, len);
	}
	// encrypt
	if (flag & COMM_UNENCRYPTED)
		pkgbuf->Flag |= COMM_UNENCRYPTED;
	else
	{
		if (CodeBook)
			Encrypt(CodeBook, pkgbuf->Load, pkgbuf->Load, len);
		else
		{
			printf("CodeBook is NULL!\n");
			return 1;
		}
	}
	// package ready
	len += sizeof(CommPackage);
	// send

//	WSABUF wsaBuf;
//	wsaBuf.buf = (char*)sbuf;
//	wsaBuf.len = size;
//	// bugs here, need more test
//	WSAOVERLAPPED wslap;
//	ZeroMemory(&wslap, sizeof(wslap));
//	int ret = WSASendTo(ClientSock, &wb, 1, &size, 0, &ServerAddr, ServerAddrLen, &wslap, NULL);

	int ret = sendto(ClientSock, (const char*)sbuf, len, 0, &ServerAddr, ServerAddrLen);
#ifdef _DEBUG
	if (ret <= 0)
		printf("%d %d\n", ret, WSAGetLastError());
#endif
	sTotalPackagesSend++;
	sTotalBytesSend += size;
	return 0;
}


int CommClient::BeginRecv()
{
	readState = COMM_READ_STATUS_NONE;
	WSABUF wsaBuf;
	wsaBuf.buf = innerBuf;
	wsaBuf.len = sizeof(innerBuf);
	DWORD flag = 0;
	int ret = WSARecvFrom(ClientSock, &wsaBuf, 1, &innerLength, &flag, &ServerAddr, &ServerAddrLen, &overlapRead, NULL);
	if (ret > 0)
	{
		readState = COMM_READ_STATUS_DONE;
		SetEvent(readEvent);
		return 0;
	}
	else if (WSAGetLastError() == WSA_IO_PENDING)
	{
		readState = COMM_READ_STATUS_PENDING;
		return 0;
	}
	else
	{
		printf("%d %d\n", ret, WSAGetLastError());
		return 1;
	}
}


void CommClient::InitRecv()
{
	readState = COMM_READ_STATUS_NONE;
	readEvent =  WSACreateEvent();
	ZeroMemory(&overlapRead, sizeof(overlapRead));
	overlapRead.hEvent = readEvent;
}

int CommClient::EndRecv(void* buf, unsigned int* size)
{
	if (readState == COMM_READ_STATUS_NONE)
		return 1;
	if (readState == COMM_READ_STATUS_PENDING)
	{
		DWORD flag = 0;
		if (WSAGetOverlappedResult(ClientSock, &overlapRead, &innerLength, FALSE, &flag) == FALSE)
		{
			printf("Err %d", WSAGetLastError());
			return 1;
		}
	}
	// else (readState == COMM_READ_STATUS_DONE)
	// do nothing
	CommPackage* pkg = (CommPackage*)innerBuf;
	// compress
	innerLength -= sizeof(CommPackage);
	// encrypt
	if (pkg->Flag & COMM_UNENCRYPTED == 0)
	{
		if (CodeBook)
			Encrypt(CodeBook, pkg->Load, pkg->Load, innerLength);
		else
		{
			printf("CodeBook is NULL!\n");
			return 1;
		}
	}
	if (pkg->Flag & COMM_COMPRESSED)
		Decompress(pkg->Load, innerLength, buf, size);
	else
	{
		memcpy(buf, pkg->Load, innerLength);
		*size = innerLength;
	}
	// read done
	sTotalPackagesRecv++;
	sTotalBytesRecv += *size;
	//
	readState = COMM_READ_STATUS_NONE;
	return 0;
}

int CommClient::InitEnvironment()
{
	WSAData wsaData;
	int ret;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup failed with error %d\n", ret);
		return 1;
	}
	return 0;
}

int CommClient::CleanEnvironment()
{
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("WSACLearup failed with error %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}
