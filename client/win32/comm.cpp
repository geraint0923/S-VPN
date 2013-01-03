//
#include "comm.h"
#include <WinSock2.h>

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
}

int CommClient::SendEncrypt(const void* buf, unsigned long size)
{
	char enbuf[PACKAGE_BUFFER_SIZE];
	Encrypt(&CodeBook, buf, enbuf, size);
	return sendto(ClientSock, enbuf, size, 0, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
}

int CommClient::RecvDecrypt(void* buf, unsigned long& size)
{
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
