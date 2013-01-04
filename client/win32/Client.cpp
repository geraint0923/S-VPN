//
#include "Client.h"
#include "tun.h"
#include "comm.h"

int Client::Initialize(Config conf)
{
	CommClient::InitEnvironment();

	conf.UseDebugConfig();

	comm = new CommClient();
	if (comm->Connect(conf.sinfo, conf.uinfo) != 0)
	{
		printf("Connecting to Server Failed.\n");
		delete comm;
		return -1;
	}

	tun = TunDriver::OpenDriver();
	if (tun == NULL)
	{
		printf("Initalize Tun Failed.\n");
		comm->Disconnect();
		delete comm;
		return -1;
	}
}

static DWORD WINAPI sendThreadRoutine(LPVOID param)
{
	return ((Client*)param)->SendCycle();
}
 
static DWORD WINAPI recvThreadRoutine(LPVOID param)
{
	return ((Client*)param)->RecvCycle();
}

int Client::SendCycle()
{
	unsigned char buf[PACKAGE_BUFFER_SIZE];
	
	char tunbuf[PACKAGE_BUFFER_SIZE];
	char netbuf[PACKAGE_BUFFER_SIZE];

	DWORD tunlen, netlen;
	
	HANDLE mwait[] = {
		comm->InitRecv(),
		tun->InitRead()
	};
	tun->BeginRead();
	comm->BeginRecvDecrypt();

	while (Running)
	{
		//WSAWaitFor
		DWORD ret = WaitForMultipleObjects(2, mwait, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0 + 1)
		{
			tun->EndRead(tunbuf, tunlen);
			
			printf("%d ae\n", tunbuf[0] >> 4);
			if (tunbuf[0] >> 4 == 4)
			{
				comm->SendEncrypt(tunbuf, tunlen);
				printf("Send a package\n");
			}
			tun->BeginRead();
	////				unsigned long len;
	//	comm->RecvDecrypt(buf, len);
	//	// filter
	//	tun->WritePackage(buf, len);
		}
		else if (ret == WAIT_OBJECT_0)
		{
			comm->EndRecvDecrypt(buf, netlen);
			// filter
			tun->Write(buf, netlen);
			comm->BeginRecvDecrypt();
		}
	}
	return 0;
}

int Client::RecvCycle()
{
	unsigned char buf[PACKAGE_BUFFER_SIZE];
	while (Running)
	{
		//unsigned long len;
		//comm->RecvDecrypt(buf, len);

		//printf("recved a package\n");
		////WaitForSingleObject(ev1, INFINITE);
		//// filter
		////memcpy(buf, debugbuf, debuglen);
		//for (int i = 0; i < 4; ++i)
		//{
		//	char tmp = buf[12 + i]; buf[12 + i] = buf[16 + i]; buf[16 + i] = tmp;
		//}
		//tun->WritePackage(buf, len);

		//SetEvent(ev2);
	}
	return 0;
}

int Client::Run()
{
	// set running flag
	Running = true;

	ev1 = CreateEvent(NULL, FALSE, FALSE, NULL);
	//ev2 = CreateEvent(NULL, FALSE, FALSE, NULL);

	// create threads
	SendThread = CreateThread(NULL, 0, sendThreadRoutine, this, 0, NULL);
	//RecvThread = CreateThread(NULL, 0, recvThreadRoutine, this, 0, NULL);
	// wait until exit
	HANDLE hls[] = {SendThread, RecvThread};
	WaitForMultipleObjects(1, hls, TRUE, INFINITE);
	//
	return 0;
}

int Client::Finalize()
{
	delete tun;
	delete comm;
	CommClient::CleanEnvironment();
	return 0;
}
