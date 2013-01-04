#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <string>
#include "tun.h"
#include "comm.h"
#include "config.h"

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

class Client
{
public:
	int Initialize(Config conf);
	int Run();
	int Finalize();
public:
	TunDriver* tun;
	CommClient* comm;

	HANDLE RecvThread;
	HANDLE SendThread;

	char debugbuf[10000];
	int debuglen;
	HANDLE ev1, ev2;

	int SendCycle();
	int RecvCycle();

	bool Running;
};



#endif