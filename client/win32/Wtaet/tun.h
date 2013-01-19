#ifndef __TUN_H__
#define __TUN_H__

#include "common.h"

class Client;

class TunDriver
{
public:
	static void QueryTunList(); // get available tun list

	TunDriver(/* TODO: tun_number */); // open a tun
	int Open(/*const sockaddr* localAddr, int flag*/); // set ip & .. and ready to use
	~TunDriver();

	int Write(const void* buf, unsigned long size);

	HANDLE InitRead();
	int BeginRead();
	void EndRead(void* buf, unsigned long& size);
public:
	Client* Current;

	fd_t fd;
	ULONG mtu;

	OVERLAPPED overlapRead;
	int readState;
	HANDLE readEvent;

	char rawBuf[10000];
	DWORD rawBufLen;

	int PackageRecvedHandler();
};

#endif
