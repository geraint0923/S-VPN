	//None = 0,
	//Init,
	//SentUserName,
	//SentPassword,
	//Connected,
	//SentKeepAlive,
	//Reconnect,
	//Error


#include "account.h"
#include "Client.h"
#include "common.h"


int Account::Login()
{
	int ret;
	// assume preconfigured
	if ((ret = Current->comm->Connect()) < 0)
		return ret;
	// run comm
	Current->comm->Running = true;
	if ((ret = SendUserName()) < 0)
		return ret;
	// ready, wait for response
	this->Status = AS_SentUserName;
	ret = Current->comm->BeginRecv();
	return ret;
}

int Account::ForwardFromTun(const unsigned char* buf, size_t size)
{
	// TODO: check package
	return Current->tun->Write(buf, size);
}

int Account::ForwardFromNet(const unsigned char* buf, size_t size, int flag)
{
	if (flag & PACKAGE_CONTROL)
		return HandleControlPackage(buf, size, flag);

}

int Account::HandleControlPackage(const unsigned char* buf, size_t size, int flag)
{
	BYTE type = *(BYTE*)data;
	switch (type)
	{
	case CONTROL_HEADER_TIMESTAMP:
		{
			if (Status != AS_SentUserName)
				break;
			// parse package
			ControlTimeStampHeader preHeader = *((ControlTimeStampHeader*)data);
			preHeader.ConvertByteOrder();
			// options
			if (preHeader.Status != SVPN_OK)
			{
				printf("Error.\n");
				break;
			}
			// update the encrypt codebook
			if (!SendPassword())
			{
				printf("Send password package error.\n");
			}
			Status = AS_SentPassword;
			SetKeepAliveTimer(COMM_TIMEOUT);
		}
		break;
	case CONTROL_HEADER_LOGININFO:
		{
			if (Status != AS_SentPassword && Status != AS_SentKeepAlive)
				break;
			// parse package
			ControlLoginInfoHeader preHeader = *((ControlLoginInfoHeader*)data);
			preHeader.ConvertByteOrder();
			// check status
			if (preHeader.Status != SVPN_OK)
			{
				printf("Error.\n");
				break;
			}
			// update server address
			Current->comm->UpdateServerAddress(&preHeader.RemoteAddress,
				&preHeader.DNSAddress,
				preHeader.Options);
			// update tun address
			Current->tun->UpdateLocalAddress(&preHeader.LocalAddresss,
				preHeader.Options);
			// set timeout for keepalive
			ValidTime = preHeader.ValidTime;
			SetKeepAliveTimer(ValidTime);
			// ready state
			Status = AS_Connected;
		}
		break;
	default:
		printf("Account Package error.\n");
		return 1;
	}
	return 0;
}

int Account::SendUserName()
{
	ControlUserNameHeader nextHeader;
	nextHeader.Type = CONTROL_HEADER_USERNAME;
	nextHeader.Options = 0;
	EncodeString(UserName.c_str(), nextHeader.UserName);
	nextHeader.ConvertByteOrder();
	//send
	return Current->comm->Send(&nextHeader, sizeof(nextHeader), COMM_UNENCRYPTED);
}

int Account::SendPassword()
{
	ControlPasswordHeader nextHeader;
	nextHeader.Type = CONTROL_HEADER_PASSWORD;
	nextHeader.Options = 0;
	nextHeader.MagicNumber = MAGIC_NUMBER;
	nextHeader.ConvertByteOrder();
	// send
	return Current->comm->Send(&nextHeader, sizeof(nextHeader), 0);
}

int Account::TimeoutHandler(HANDLE hEvent)
{
	int ret;
	switch (Status)
	{
	case AS_SentUserName:
		{
			ret = SendUserName();
			SetKeepAliveTimer(COMM_TIMEOUT);
		}
		break;
	case AS_SentPassword:
	case AS_SentKeepAlive:
	case AS_Connected:
		{
			ret = SendPassword();
			SetKeepAliveTimer(COMM_TIMEOUT);
		}
		break;
	}
	return ret;
}

void Account::SetKeepAliveTimer(unsigned int timeout)
{
	int ret;
	LARGE_INTEGER dueTime;
	dueTime.QuadPart = timeout;
	CancelWaitableTimer(KeepAliveTimer);
	ret = SetWaitableTimer(KeepAliveTimer, &dueTime, 0, NULL, NULL, FALSE);
	if (ret == FALSE)
		printf("Error.\n");
	Current->RegisterEvent(KeepAliveTimer, this, (WaitHandler)Account::TimeoutHandler);
}
