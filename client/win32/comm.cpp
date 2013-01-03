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
	ClientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	//WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, 0); // Overlapped set here

	// bind local address
	sockaddr_in localAddr;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(0);
	bind(ClientSock, (sockaddr*)&localAddr, sizeof(sockaddr_in));

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
	return sendto(ClientSock, enbuf, size, 0, &ServerAddr, ServerAddrLen);
}

int CommClient::RecvDecrypt(void* buf, unsigned long& size)
{
	char debuf[PACKAGE_BUFFER_SIZE];
	int ret = recvfrom(ClientSock, debuf, PACKAGE_BUFFER_SIZE, 0, &ServerAddr, &ServerAddrLen);
//		printf("Error : %d \n", WSAGetLastError());
//	gethostbyname("")->;
	if (ret < 0)
		return ret;
	Decrypt(&CodeBook, debuf, buf, ret);
	size = ret;
	return 0;
}

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
