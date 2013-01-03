#ifndef __TUN_H__
#define __TUN_H__

#ifndef PACKAGE_BUFFER_SIZE
#define PACKAGE_BUFFER_SIZE 10000
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

class TunDriver
{
public:
	static TunDriver* OpenDriver(int ID = 0);

	TunDriver();
	~TunDriver();

	int ReadPackage(void* buf, unsigned long& size);
	int WritePackage(const void* buf, unsigned long size);
public:
	HANDLE fd;
	ULONG mtu;
};

#endif
