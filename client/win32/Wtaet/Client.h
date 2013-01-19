#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <string>
#include "tun.h"
#include "comm.h"
#include "account.h"
#include "config.h"
#include "common.h"

#define MY_MAXIMUM_WAIT_OBJECTS 10

class Client
{
public:
	int Initialize(Config conf);
	int Finalize();

	// WaitQueue
	int RegisterEvent(HANDLE hEvent, Waitable* object, WaitHandler handler);
	int UnregisterEvent(HANDLE hEvent);

	void ClearEventQueue();

	int EventLoop();
public:
	TunDriver* tun;
	CommClient* comm;
	Account* account;
//	Console* console;

private:
	// WaitHandler WaitQueue
	unsigned int WaitQueueSize;
	HANDLE WaitQueue[MY_MAXIMUM_WAIT_OBJECTS];
	struct
	{
		Waitable* Object;
		WaitHandler Handler;
	}
	WaitQueueHandler[MY_MAXIMUM_WAIT_OBJECTS];

	bool Running;
};

#endif