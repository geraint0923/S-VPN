//
#include "comm.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
//33333
// a 0
// www.huxx.me

CommClient::CommClient()
{
	Status = Init;
}

CommClient::~CommClient()
{
}

int CommClient::Connect(const ServerInfo& sinfo, const UserInfo& uinfo)
{
	if (Status != Init && Status != Disconnected)
		return -1;
	// dns solve
	char strbuf[8];
	sprintf_s(strbuf, sizeof(strbuf), "%d", sinfo.ServerPort);
	ADDRINFOA hints;
	ZeroMemory(&hints, sizeof(hints));
    hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_protocol = IPPROTO_TCP;
	PADDRINFOA addrInfo;
	getaddrinfo(sinfo.ServerName.c_str(), strbuf, &hints, &addrInfo);
	if (addrInfo == NULL)
	{
		printf("Error resolving server address.\n");
		return -1;
	}
	ServerAddrLen = addrInfo->ai_addrlen;
	ServerAddr = *addrInfo->ai_addr;
//	printf("Sever addr : %s %d\n", inet_ntoa(((sockaddr_in*)&ServerAddr)->sin_addr), ntohs(((sockaddr_in*)&ServerAddr)->sin_port));
	freeaddrinfo(addrInfo);

	ServerAddr2 = ServerAddr;
	ServerAddr2.sa_data[1]++;

	// create socket
	//ClientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	ClientSock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); // Overlapped set here
//	ClientSock2 = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); // Overlapped set here
//	ClientSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); // Overlapped set here
//	printf("Connect: %d\n", connect(ClientSock, &ServerAddr, ServerAddrLen));

	int sendbuff = 1024*4;
//	socklen_t optlen = sizeof(sendbuff);
	int ree = setsockopt(ClientSock, SOL_SOCKET, SO_SNDBUF, (char*)&sendbuff, sizeof(sendbuff));
//	sendbuff = 1024*128;
//	ree = setsockopt(ClientSock, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuff, optlen);
	// bind local address
	sockaddr_in localAddr;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(33333);
	bind(ClientSock, (sockaddr*)&localAddr, sizeof(sockaddr_in));
//	localAddr.sin_port = htons(33334);
//	bind(ClientSock2, (sockaddr*)&localAddr, sizeof(sockaddr_in));

	char tmpbuf[10000];
	int tmpsize = 1460;
//	sendto(ClientSock, tmpbuf, tmpsize, 0, &ServerAddr, ServerAddrLen);

	// build codebook
	BuildTable(&CodeBook, uinfo.MD5, 0LL);
	return 0;
}

int CommClient::Disconnect()
{
	return 0;
}

int cnnt = 0;

int CommClient::SendEncrypt(const void* buf, unsigned long size)
{
	cnnt++;
	char enbuf[PACKAGE_BUFFER_SIZE];
	Encrypt(&CodeBook, buf, enbuf, size);
	WSABUF wb;
	wb.buf = enbuf;
	wb.len = size;
	WSAOVERLAPPED wslap;
	ZeroMemory(&wslap, sizeof(wslap));
//	int ret = WSASendTo(ClientSock, &wb, 1, &size, 0, &ServerAddr, ServerAddrLen, &wslap, NULL);
	int ret;
//	if (cnnt % 2 == 0)
		ret =  sendto(ClientSock, enbuf, size, 0, &ServerAddr, ServerAddrLen);
//	else
//		ret =  sendto(ClientSock2, enbuf, size, 0, &ServerAddr2, ServerAddrLen);

	printf("Comm Send %d\n", cnnt % 2 + 1);

	//int ret =  send(ClientSock, enbuf, size + 4, 0);
	printf("%d %d\n", ret, WSAGetLastError());
	return 0;
}

HANDLE CommClient::InitRecv()
{
	readState = 0;
	readEvent =  WSACreateEvent();//CreateEvent(NULL, FALSE, FALSE, NULL); //
	ZeroMemory(&overlapRead, sizeof(overlapRead));
	overlapRead.hEvent = readEvent;
	return readEvent;
}

//////
//////
//////

HANDLE CommClient::InitRecv2()
{
	readState2 = 0;
	readEvent2 =  WSACreateEvent();//CreateEvent(NULL, FALSE, FALSE, NULL); //
	ZeroMemory(&overlapRead2, sizeof(overlapRead2));
	overlapRead2.hEvent = readEvent2;
	return readEvent2;
}

int CommClient::BeginRecvDecrypt2()
{
	WSABUF wb;
	wb.buf = innerBuf2;
	innerLength2 = 10000;
	wb.len = sizeof(innerBuf2);
	ServerAddrLen = sizeof(sockaddr_in);
	DWORD flag = 0;
	int ret = WSARecvFrom(ClientSock2, &wb, 1, &innerLength2, &flag, &ServerAddr2, &ServerAddrLen, &overlapRead2, NULL);
	//int ret = WSARecv(ClientSock, &wb, 1, &inl, &flag, &overlapRead, NULL);
	if (ret > 0)
	{
		readState2 = 1;
		SetEvent(readEvent2);
		return 0;
	}
	else if (WSAGetLastError() == WSA_IO_PENDING)
	{
		readState2 = 0;
		return 0;
	}
	else
	{
		printf("%d %d\n", ret, WSAGetLastError());
		return -1;
	}
}

int CommClient::EndRecvDecrypt2(void* buf, unsigned long& size)
{
	if (readState2 == 1)
	{
		Decrypt(&CodeBook, innerBuf2, buf, innerLength2);
		memcpy(buf, innerBuf2, innerLength2);
		size = innerLength2;
	}
	else
	{
		DWORD flag = 0;
		DWORD rlen;
		if (WSAGetOverlappedResult(ClientSock2, &overlapRead2, &rlen, FALSE, &flag))
		{
			innerLength2 = rlen;
			Decrypt(&CodeBook, innerBuf2, buf, innerLength2);
			size = innerLength2;
		}
		else
		{
			printf("Err2 %d", WSAGetLastError());
			return -1;
			//::perror("Someoitoaisdnfas\n");
		}
	}
	readState2 = 0;
	return 0;
}
//////
//////
//////

int CommClient::BeginRecvDecrypt()
{
	WSABUF wb;
	wb.buf = innerBuf;
	innerLength = 10000;
	wb.len = sizeof(innerBuf);
	ServerAddrLen = sizeof(sockaddr_in);
	DWORD flag = 0;
	int ret = WSARecvFrom(ClientSock, &wb, 1, &innerLength, &flag, &ServerAddr, &ServerAddrLen, &overlapRead, NULL);
	//int ret = WSARecv(ClientSock, &wb, 1, &inl, &flag, &overlapRead, NULL);
	if (ret > 0)
	{
		readState = 1;
		SetEvent(readEvent);
		return 0;
	}
	else if (WSAGetLastError() == WSA_IO_PENDING)
	{
		readState = 0;
		return 0;
	}
	else
	{
		printf("%d %d\n", ret, WSAGetLastError());
		return -1;
	}
}

int CommClient::EndRecvDecrypt(void* buf, unsigned long& size)
{
	if (readState == 1)
	{
		Decrypt(&CodeBook, innerBuf, buf, innerLength);
		memcpy(buf, innerBuf, innerLength);
		size = innerLength;
	}
	else
	{
		DWORD flag = 0;
		DWORD rlen;
		if (WSAGetOverlappedResult(ClientSock, &overlapRead, &rlen, FALSE, &flag))
		{
			innerLength = rlen;
			Decrypt(&CodeBook, innerBuf, buf, innerLength);
			size = innerLength;
		}
		else
		{
			printf("Err %d", WSAGetLastError());
			return -1;
			//::perror("Someoitoaisdnfas\n");
		}
	}
	readState = 0;
	return 0;
}

//int CommClient::RecvDecrypt(void* buf, unsigned long& size)
//{
//	char debuf[PACKAGE_BUFFER_SIZE];
//	sockaddr saddr = ServerAddr;
//	int saddrl = ServerAddrLen;
//	int ret = recvfrom(ClientSock, debuf, PACKAGE_BUFFER_SIZE, 0, &saddr, &saddrl);
////		printf("Error : %d \n", WSAGetLastError());
////	gethostbyname("")->;
//	if (ret < 0)
//		return ret;
//	Decrypt(&CodeBook, debuf, buf, ret);
//	size = ret;
//	return 0;
//}

int CommClient::InitEnvironment()
{
	WSAData wsaData;
	int ret;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup failed with error %d\n", ret);
		return -1;
	}
	return 0;
}

int CommClient::CleanEnvironment()
{
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("WSACLearup failed with error %d\n", WSAGetLastError());
		return -1;
	}
	return 0;
}
