#ifndef __LOGIN_H__
#define __LOGIN_H__

#include "config.h"
#include <WinSock2.h>

class Client;

enum AccountStatus
{
	AS_None = 0,
	AS_Init,
	AS_SentUserName,
	AS_SentPassword,
	AS_Connected,
	AS_SentKeepAlive,
	AS_Reconnect,
	AS_Error
};

class Account : public Waitable
{
public:
	// user account info
	TSTRING UserName;
	unsigned char PasswordMD5[16];
	
	Client* Current;
	AccountStatus Status;

	unsigned int ValidTime;
	HANDLE KeepAliveTimer;
public:
	Account(Client* current);

	int Login();

	int TimeoutHandler(HANDLE hEvent);
	int PackageHandler(const void* data, int length);

	void SetKeepAliveTimer(unsigned int timeout);

	int SendUserName();
	int SendPassword();

	int ForwardFromTun(const unsigned char* buf, size_t size);
	int ForwardFromNet(const unsigned char* buf, size_t size, int flag);

	int HandleControlPackage(const unsigned char* buf, size_t size, int flag);
};

#endif
