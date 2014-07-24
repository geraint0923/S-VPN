//
#include <tchar.h>
#include <WinSock2.h>
#include <WinIoCtl.h>
#include <WinReg.h>

#include <stdio.h>

#include "tun.h"

//=============
// TAP IOCTLs
//=============

#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

// Present in 8.1

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)

// Added in 8.2
/* obsoletes TAP_IOCTL_CONFIG_POINT_TO_POINT */
#define TAP_IOCTL_CONFIG_TUN            TAP_CONTROL_CODE (10, METHOD_BUFFERED)

//=================
// Registry keys
//=================

#define ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define NETWORK_CONNECTIONS_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define NETWORK_CONNECTION_KEY_SURFIX "\\Connection"

//======================
// Filesystem prefixes
//======================

#define USERMODEDEVICEDIR "\\\\.\\Global\\"
#define TAPSUFFIX         ".tap"

//=========================================================
// TAP_COMPONENT_ID -- This string defines the TAP driver
// type -- different component IDs can reside in the system
// simultaneously.
//=========================================================

#define TAP_COMPONENT_ID TAP_ID

#define COMPONENT_ID "ComponentId"
#define TARGET_COMPONENT_ID "tap0901"
#define NET_CFG_INSTANCE_ID "NetCfgInstanceId"
#define CONNECTION_NAME "Name"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255

#define MAX_TUN_ADAPTERS 7

struct TunAdapterInfo
{
	TCHAR Guid[MAX_VALUE_NAME];
	TCHAR FriendlyName[MAX_VALUE_NAME];
};

struct TunAdaptersInfo
{
	int Count;
	TunAdapterInfo Adapters[MAX_TUN_ADAPTERS];
};

void QueryTunAdapterInfo(TunAdaptersInfo* info)
{ 
	info->Count = 0;

	LSTATUS ret;
	HKEY hKeyAdapters;
	if ((ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(ADAPTER_KEY), 0,
		KEY_READ, &hKeyAdapters)) != ERROR_SUCCESS)
	{
		fprintf(stderr, "[Error] Query adapters failed, ret = %d\n", ret);
		return;
	}

	HKEY hKeyConnections;
	if ((ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(NETWORK_CONNECTIONS_KEY), 0,
		KEY_READ, &hKeyConnections)) != ERROR_SUCCESS)
	{
		fprintf(stderr, "[Error] Query adapters failed, ret = %d\n", ret);
		RegCloseKey(hKeyAdapters);
		return;
	}

    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
	DWORD keyCount;
	if ((ret = RegQueryInfoKey(hKeyAdapters, NULL, NULL, NULL,
		&keyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) != ERROR_SUCCESS)
	{
		fprintf(stderr, "[Error] Query adapters failed, ret = %d\n", ret);
		RegCloseKey(hKeyConnections);
		RegCloseKey(hKeyAdapters);
		return;
	}

	for (unsigned int i = 0; i < keyCount; i++)
	{
		TCHAR keyName[MAX_KEY_LENGTH];
		if ((ret = RegEnumKey(hKeyAdapters, i, keyName, MAX_KEY_LENGTH)) != ERROR_SUCCESS)
			continue;

		HKEY subKey;
		if ((ret = RegOpenKeyEx(hKeyAdapters, keyName, 0, KEY_READ, &subKey)) != ERROR_SUCCESS)
			continue;

		BYTE compID[MAX_VALUE_NAME];
		DWORD cbData = MAX_VALUE_NAME;
		if ((ret = RegQueryValueEx(subKey, TEXT(COMPONENT_ID), NULL, NULL,
			compID, &cbData)) != ERROR_SUCCESS)
		{
			RegCloseKey(subKey);
			continue;
		}
		// match compid with target
		const TCHAR* compIDName = (const TCHAR*)compID;
		if (_tcscmp(compIDName, TEXT(TARGET_COMPONENT_ID)) == 0)
		{
			// get guid
			BYTE netCfgID[MAX_VALUE_NAME];
			cbData = MAX_VALUE_NAME;
			if ((ret = RegQueryValueEx(subKey, TEXT(NET_CFG_INSTANCE_ID),
				NULL, NULL, netCfgID, &cbData)) != ERROR_SUCCESS)
			{
				RegCloseKey(subKey);
				continue;
			}
			memcpy(&info->Adapters[info->Count].Guid, netCfgID, cbData);

			TCHAR* connKeySurfix = (TCHAR*)netCfgID;
			// get friendly name
			HKEY connKey;
			_tcscat(connKeySurfix, TEXT(NETWORK_CONNECTION_KEY_SURFIX));
			if ((ret = RegOpenKeyEx(hKeyConnections, connKeySurfix, 0, KEY_READ,
				&connKey)) != ERROR_SUCCESS)
			{
				RegCloseKey(subKey);
				continue;
			}

			BYTE connName[MAX_VALUE_NAME];
			cbData = MAX_VALUE_NAME;
			if ((ret = RegQueryValueEx(connKey, TEXT(CONNECTION_NAME),
				NULL, NULL, connName, &cbData)) != ERROR_SUCCESS)
			{
				RegCloseKey(connKey);
				RegCloseKey(subKey);
				continue;
			}
			memcpy(&info->Adapters[info->Count].FriendlyName, connName, cbData);

			RegCloseKey(connKey);
			// add info count
			info->Count++;
		}
		RegCloseKey(subKey);
    }
	RegCloseKey(hKeyConnections);
	RegCloseKey(hKeyAdapters);
}


TunDriver* TunDriver::OpenDriver(int ID)
{
	TunAdaptersInfo tunAdaptsInfo;
	QueryTunAdapterInfo(&tunAdaptsInfo);
	for (int i = 0 ; i < tunAdaptsInfo.Count; i++)
	{
		printf("%ls %ls\n", tunAdaptsInfo.Adapters[i].Guid,
			tunAdaptsInfo.Adapters[i].FriendlyName);
	}

	const char* guid = "{A338319D-9D15-4178-A32B-D012DF84FF1E}";
	// open tun device file
	char fileName[256];
	sprintf(fileName, "%s%s%s", USERMODEDEVICEDIR, guid, TAPSUFFIX);
	HANDLE fd = CreateFileA(fileName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
		0);
	if (fd == INVALID_HANDLE_VALUE)
	{
		printf("Device File Could not be opened.\n");
		return NULL;
	}

	// check version
	DWORD len;
	ULONG versionInfo[3];
	ZeroMemory(versionInfo, sizeof(versionInfo));
    DeviceIoControl(fd, TAP_IOCTL_GET_VERSION, &versionInfo, sizeof(versionInfo), &versionInfo, sizeof(versionInfo), &len, NULL);
	if (versionInfo[0] != 9 || versionInfo[1] != 9)
	{
		printf("TAP-Win32 9.9 required. Current is %d.%d %s.\n", (int)versionInfo[0], (int)versionInfo[1], (versionInfo[2] ? "(DEBUG)" : ""));
		CloseHandle(fd);
		return NULL;
	}

	// check MTU
	ULONG mtu;
    DeviceIoControl (fd, TAP_IOCTL_GET_MTU, &mtu, sizeof(mtu), &mtu, sizeof(mtu), &len, NULL);
	printf("TAP-Win32 MTU = %u\n", mtu);

	// FIXME:
	// set addresses
	{
		DWORD ep[3];
		ep[0] = inet_addr("192.168.3.6");
		ep[1] = inet_addr("192.168.3.0");
		ep[2] = inet_addr("255.255.255.0");

	//		ep[0] = inet_addr("10.3.0.1");
	//ep[1] = inet_addr("10.3.0.0");
	//ep[2] = inet_addr("255.255.255.0");

		BOOL statusd = DeviceIoControl(fd, TAP_IOCTL_CONFIG_TUN, ep, sizeof(ep), ep, sizeof(ep), &len, NULL);
		if (statusd == FALSE)
		{
			printf("Set TUN IPaddress Failed.\n");
			CloseHandle(fd);
			return NULL;
		}
	}

	// set device status connected
	{
		ULONG status = TRUE;
		if (!DeviceIoControl(fd, TAP_IOCTL_SET_MEDIA_STATUS, &status, sizeof (status), &status, sizeof (status), &len, NULL))
		{
			printf("WARNING: The TAP-Win32 driver rejected a TAP_IOCTL_SET_MEDIA_STATUS DeviceIoControl call.\n");
			CloseHandle(fd);
			return NULL;
		}
	}
	
	// create new driver struct
	TunDriver* tun = new TunDriver();
	tun->fd = fd;
	tun->mtu = mtu;
	return tun;
}

TunDriver::TunDriver()
{
}

//int TunDriver::ReadPackage(void* buf, unsigned long* psize)
//{
//	BOOL ret = ReadFile(fd, buf, PACKAGE_BUFFER_SIZE, psize, NULL);
//	return  (ret == FALSE) ? -1 : 0;
//}

HANDLE TunDriver::InitRead()
{
	readState = 0; // no data
	readEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	ZeroMemory(&overlapRead, sizeof(overlapRead));
	overlapRead.hEvent = readEvent;
	return readEvent;
}

int TunDriver::BeginRead()
{
	if (ReadFile(fd, innerBuf, 10000, &innerLength, &overlapRead))
	{
		readState = 1;
		SetEvent(readEvent);
		return 0;
	}
	else if (GetLastError() == ERROR_IO_PENDING)
	{
		readState = 0;
		return 0;
	}
	else
		return -1;
}

void TunDriver::EndRead(void* buf, unsigned long& size)
{
	if (readState == 1)
	{
		memcpy(buf, innerBuf, innerLength);
		size = innerLength;
	}
	else
	{
		if (GetOverlappedResult(fd, &overlapRead, &innerLength, FALSE))
		{
			memcpy(buf, innerBuf, innerLength);
			size = innerLength;
		}
		else
			::perror("Something just not right.\n");
	}
	readState = 0;
}

int TunDriver::Write(const void* buf, unsigned long size)
{
	unsigned long written;
	OVERLAPPED olp;
	ZeroMemory(&olp, sizeof(olp));
	BOOL ret = WriteFile(fd, buf, size, &written, &olp);
//	printf("Err %d %d\n", ret, GetLastError());
	if (ret == FALSE || written != size)
		return -1;
	else
		return 0;
}

TunDriver::~TunDriver()
{
}
