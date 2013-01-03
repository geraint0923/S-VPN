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

	tun = new TunDriver();
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
	while (Running)
	{
		unsigned long len;
		tun->ReadPackage(buf, len);
		// filter ipv4 package
		if (buf[0] >> 4 == 4)
		{
			comm->SendEncrypt(buf, len);
		}
	}
	return 0;
}

int Client::RecvCycle()
{
	unsigned char buf[PACKAGE_BUFFER_SIZE];
	while (Running)
	{
		unsigned long len;
		comm->RecvDecrypt(buf, len);
		// filter
		tun->WritePackage(buf, len);
	}
	return 0;
}

int Client::Run()
{
	// set running flag
	Running = true;
	// create threads
	SendThread = CreateThread(NULL, 0, sendThreadRoutine, this, 0, NULL);
	RecvThread = CreateThread(NULL, 0, recvThreadRoutine, this, 0, NULL);
	// wait until exit
	HANDLE hls[] = {SendThread, RecvThread};
	WaitForMultipleObjects(2, hls, TRUE, INFINITE);
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
