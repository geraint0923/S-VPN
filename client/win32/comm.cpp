//
#include "comm.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>

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
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
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

	// create socket
	//ClientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	ClientSock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); // Overlapped set here

	// bind local address
	sockaddr_in localAddr;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(0);
	bind(ClientSock, (sockaddr*)&localAddr, sizeof(sockaddr_in));

	// build codebook
	BuildTable(&CodeBook, uinfo.MD5, 0LL);
	return 0;
}

int CommClient::Disconnect()
{
	return 0;
}

int CommClient::SendEncrypt(const void* buf, unsigned long size)
{
	char enbuf[PACKAGE_BUFFER_SIZE];
	Encrypt(&CodeBook, buf, enbuf, size);
	//WSABUF wb;
	//wb.buf = (char*)enbuf;
	//wb.len = size;
	//DWORD tmp;
	//int ret = WSASendTo(ClientSock, &wb, 1, &size, 0, &ServerAddr, ServerAddrLen, &overlapRead, NULL);
	int ret =  sendto(ClientSock, enbuf, size, 0, &ServerAddr, ServerAddrLen);
	//printf("%d %d\n", ret, WSAGetLastError());
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

int CommClient::BeginRecvDecrypt()
{
	WSABUF wb;
	wb.buf = innerBuf;
	wb.len = sizeof(innerBuf);
	innerLength = 10000;
	ServerAddrLen = sizeof(sockaddr_in);
	DWORD flag = 0;
	int ret = WSARecvFrom(ClientSock, &wb, 1, &innerLength, &flag, &ServerAddr, &ServerAddrLen, &overlapRead, NULL);
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
		if (WSAGetOverlappedResult(ClientSock, &overlapRead, &innerLength, FALSE, &flag))
		{
			Decrypt(&CodeBook, innerBuf, buf, innerLength);
		//	memcpy(buf, innerBuf, innerLength);
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
