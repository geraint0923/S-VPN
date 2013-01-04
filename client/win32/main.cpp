#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinIoCtl.h>
#include "md5.h"
#include "crypt.h"
#include "comm.h"
#include "tun.h"
#include "Client.h"

//#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")


int main(int argc, char** argv)
{
	Client cl;
	cl.Initialize(Config());
	cl.Run();
	cl.Finalize();

//	TunDriver* tun = TunDriver::OpenDriver();
//	if (tun == NULL)
//		return 0;
	system("pause");
	return 0;
////	 FIXED_INFO *pFixedInfo;
////    IP_ADDR_STRING *pIPAddr;
////
////    ULONG ulOutBufLen;
////    DWORD dwRetVal;
////	 pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
////    ulOutBufLen = sizeof (FIXED_INFO);
////
////	 if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
////        free(pFixedInfo);
////        pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
////        if (pFixedInfo == NULL) {
////            printf("Error allocating memory needed to call GetNetworkParams\n");
////        }
////    }
////
////	     if (dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen) != NO_ERROR) {
////        printf("GetNetworkParams failed with error %d\n", dwRetVal);
////        if (pFixedInfo) {
////            free(pFixedInfo);
////        }
////    }        
////
////		        printf("\tHost Name: %s\n", pFixedInfo->HostName);
////        printf("\tDomain Name: %s\n", pFixedInfo->DomainName);
////        printf("\tDNS Servers:\n");
////        printf("\t\t%s\n", pFixedInfo->DnsServerList.IpAddress.String);
////
////        pIPAddr = pFixedInfo->DnsServerList.Next;
////        while (pIPAddr) {
////            printf("\t\t%s\n", pIPAddr->IpAddress.String);
////            pIPAddr = pIPAddr->Next;
////        }
////
////        printf("\tNode Type: ");
////        switch (pFixedInfo->NodeType) {
////        case 1:
////            printf("%s\n", "Broadcast");
////            break;
////        case 2:
////            printf("%s\n", "Peer to peer");
////            break;
////        case 4:
////            printf("%s\n", "Mixed");
////            break;
////        case 8:
////            printf("%s\n", "Hybrid");
////            break;
////        default:
////            printf("\n");
////        }
////
////        printf("\tNetBIOS Scope ID: %s\n", pFixedInfo->ScopeId);
////
////        if (pFixedInfo->EnableRouting)
////            printf("\tIP Routing Enabled: Yes\n");
////        else
////            printf("\tIP Routing Enabled: No\n");
////
////        if (pFixedInfo->EnableProxy)
////            printf("\tWINS Proxy Enabled: Yes\n");
////        else
////            printf("\tWINS Proxy Enabled: No\n");
////
////        if (pFixedInfo->EnableDns)
////            printf("\tNetBIOS Resolution Uses DNS: Yes\n");
////        else
////            printf("\tNetBIOS Resolution Uses DNS: No\n");
////
////		
////    if (pFixedInfo) {
////        free(pFixedInfo);
////        pFixedInfo = NULL;
////    }
////
////
////	IP_ADAPTER_INFO  *pAdapterInfo;
////
////pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
////ulOutBufLen = sizeof(IP_ADAPTER_INFO);
////
////
////if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
////    free (pAdapterInfo);
////    pAdapterInfo = (IP_ADAPTER_INFO *) malloc ( ulOutBufLen );
////}
////
////
////if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
////    printf("GetAdaptersInfo call failed with %d\n", dwRetVal);
////}
////
////
////
////PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
////while (pAdapter) {
////    printf("Adapter Name: %s\n", pAdapter->AdapterName);
////    printf("Adapter Desc: %s\n", pAdapter->Description);
////    printf("\tAdapter Addr: \t");
////    for (UINT i = 0; i < pAdapter->AddressLength; i++) {
////        if (i == (pAdapter->AddressLength - 1))
////            printf("%.2X\n",(int)pAdapter->Address[i]);
////        else
////            printf("%.2X-",(int)pAdapter->Address[i]);
////    }
////    printf("IP Address: %s\n", pAdapter->IpAddressList.IpAddress.String);
////    printf("IP Mask: %s\n", pAdapter->IpAddressList.IpMask.String);
////    printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
////    printf("\t***\n");
////    if (pAdapter->DhcpEnabled) {
////        printf("\tDHCP Enabled: Yes\n");
////        printf("\t\tDHCP Server: \t%s\n", pAdapter->DhcpServer.IpAddress.String);
////    }
////    else
////      printf("\tDHCP Enabled: No\n");
////
////  pAdapter = pAdapter->Next;
////}
////
////if (pAdapterInfo)
////        free(pAdapterInfo);
////
////{
////    unsigned int       i;
////
////    IP_INTERFACE_INFO*  pInterfaceInfo;
////
////	    pInterfaceInfo = (IP_INTERFACE_INFO *) malloc(sizeof (IP_INTERFACE_INFO));
////    ulOutBufLen = sizeof(IP_INTERFACE_INFO);
////
////	    if (GetInterfaceInfo(pInterfaceInfo, &ulOutBufLen) ==
////        ERROR_INSUFFICIENT_BUFFER) {
////        free(pInterfaceInfo);
////        pInterfaceInfo = (IP_INTERFACE_INFO *) malloc(ulOutBufLen);
////    }
////		    if ((dwRetVal = GetInterfaceInfo(pInterfaceInfo, &ulOutBufLen)) != NO_ERROR) {
////        printf("  GetInterfaceInfo failed with error: %d\n", dwRetVal);
////    }
////        printf("  GetInterfaceInfo succeeded.\n");
////
////        printf("  Num Adapters: %ld\n\n", pInterfaceInfo->NumAdapters);
////        for (i = 0; i < (unsigned int) pInterfaceInfo->NumAdapters; i++) {
////            printf("  Adapter Index[%d]: %ld\n", i,
////                   pInterfaceInfo->Adapter[i].Index);
////            printf("  Adapter Name[%d]:  %ws\n\n", i,
////                   pInterfaceInfo->Adapter[i].Name);
////        }
////
////	    if (pInterfaceInfo) {
////        free(pInterfaceInfo);
////        pInterfaceInfo = NULL;
////		}	
////}
////
////{
////	MIB_IPADDRTABLE  *pIPAddrTable;
////DWORD            dwSize = 0;
////DWORD            dwRetVal;
////
////pIPAddrTable = (MIB_IPADDRTABLE*) malloc( sizeof(MIB_IPADDRTABLE) );
////
////if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
////    free( pIPAddrTable );
////    pIPAddrTable = (MIB_IPADDRTABLE *) malloc ( dwSize );
////}
////if ( (dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0 )) != NO_ERROR ) { 
////    printf("GetIpAddrTable call failed with %d\n", dwRetVal);
////}
////for (int x = 0; x < pIPAddrTable->dwNumEntries; x++)
////{
////printf("IP Address:         %s\n", inet_ntoa(*((in_addr*)&(pIPAddrTable->table[x].dwAddr))));
////printf("IP Mask:            %s\n", inet_ntoa(*((in_addr*)&(pIPAddrTable->table[x].dwMask))));
////printf("IF Index:           %s\n", inet_ntoa(*((in_addr*)&(pIPAddrTable->table[x].dwIndex))));
////printf("Broadcast Addr:     %s\n", inet_ntoa(*((in_addr*)&(pIPAddrTable->table[x].dwBCastAddr))));
////printf("Re-assembly size:   %ld\n\n", pIPAddrTable->table[x].dwReasmSize);
////
////}
////if (pIPAddrTable)
////        free(pIPAddrTable);
////
////
////}
//
//	//MD5_CTX md5;
//	//MD5Init(&md5);         
//	int i;
//	char encrypt[] ="a";//21232f297a57a5a743894a0e4a801fc3
//	unsigned char md[16]; 
//
//	MD5Fast(encrypt, strlen(encrypt), md);
//
//	for (int i = 0; i < 16; i++)
//		printf("%02X ", md[i]);
//	printf("\n");
//
//	CodeTable ct;
//	BuildTable(&ct, md, 0LL);
//	char* w = "hello world.\n";
//	char outd[1000];
//	Encrypt(&ct, w, outd, strlen(w));
//	outd[strlen(w)] = 0;
//
//	printf("%s\n", outd);
//
//	Decrypt(&ct, outd, outd, strlen(w));
//
//	printf("%s\n", outd);
//
////	getchar();
//
////	return 0;
//
//
//	HANDLE fd = CreateFile(L"\\\\.\\Global\\{A338319D-9D15-4178-A32B-D012DF84FF1E}.tap",
//		GENERIC_READ | GENERIC_WRITE,
//		0,
//		0,
//		OPEN_EXISTING,
//		FILE_ATTRIBUTE_SYSTEM,// | FILE_FLAG_OVERLAPPED,
//		0);
//	if (fd == INVALID_HANDLE_VALUE)
//		printf("error!");
//
//	ULONG info[3];
//	DWORD len;
//	ZeroMemory(info, sizeof(info));
//    if (DeviceIoControl(fd, TAP_IOCTL_GET_VERSION,
//		&info, sizeof (info),
//		&info, sizeof (info), &len, NULL))
//	{
//		printf("Version : %d.%d %s\n", (int)info[0], (int)info[1], (info[2] ? "(DEBUG)" : ""));
//	}
//
//
//	ULONG mtu;
//    if (DeviceIoControl (fd, TAP_IOCTL_GET_MTU,
//			 &mtu, sizeof(mtu),
//			 &mtu, sizeof(mtu), &len, NULL))
//    {
//		printf("TAP-Win32 MTU=%d\n", (int)mtu);
//    }
//
//	DWORD ep[3];
//	ep[0] = inet_addr("10.3.0.1");
//	ep[1] = inet_addr("10.3.0.0");
//	ep[2] = inet_addr("255.255.255.0");
//
//	printf("%08x, %08x, %08x\n", ep[0], ep[1], ep[2]);
//
//	{
//		BOOL statusd = DeviceIoControl(fd, TAP_IOCTL_CONFIG_TUN,
//				ep, sizeof(ep),
//				ep, sizeof(ep), &len, NULL);
//		printf("Set TUN %s\n", statusd ? "OK": "Failed");
//	}
//
//	{
//		ULONG status = TRUE;
//		if (!DeviceIoControl(fd, TAP_IOCTL_SET_MEDIA_STATUS,
//				  &status, sizeof (status),
//				  &status, sizeof (status), &len, NULL))
//		  printf("WARNING: The TAP-Win32 driver rejected a TAP_IOCTL_SET_MEDIA_STATUS DeviceIoControl call.\n");
//	}
//
//	CommClient::InitEnvironment();
//
//	sockaddr_in local;
//	sockaddr_in from;
//	int fromlen = sizeof(from);
//	local.sin_family = AF_INET;
//	local.sin_port = htons(33334);
//	local.sin_addr.S_un.S_addr = INADDR_ANY;
//
//	from.sin_family = AF_INET;
//	from.sin_port = htons(33333);
//	from.sin_addr.S_un.S_addr = inet_addr("36.54.3.49");
//
//	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
//	bind(sock, (sockaddr*)&local, sizeof(local));
//
//	while (1)
//	{
//		OVERLAPPED lap;
//		ZeroMemory(&lap, sizeof(lap));
//
//		char* buf = (char*)malloc(20000);
//		if (tun->ReadPackage(buf, len) == 0)
//		//printf("Got %d Bytes\n", len);
//		
//		//if (GetLastError() == ERROR_IO_PENDING)
//		{
//			DWORD numread = len;
//        //    BOOL res = GetOverlappedResult(fd, &lap, &numread, TRUE);
//
//			int ver = buf[0] >> 4;
//			if (ver != 4)
//				continue;
//
//			printf("Read %d Bytes\n", numread);
//
//			//for (int i = 0; i < 4; ++i)
//			//{
//			//	char tmp = buf[12 + i]; buf[12 + i] = buf[16 + i]; buf[16 + i] = tmp;
//			//}
//
//			char buf2[20000];
//			char buf3[20000];
//			DWORD numread2;
//			Encrypt(&ct, buf, buf2, numread);
//
//			sendto(sock, buf2, numread, 0, (sockaddr*)&from, fromlen);
//
//			if (recvfrom(sock, buf3, numread, 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR)
//			{
//				printf("SDFare!!\n");
//			}
//
//			Decrypt(&ct, buf3, buf2, numread);
//
//			tun->WritePackage(buf2, numread);
//         }
//		else
//		{
//			printf("Error.\n");
//		}
//	}
//
//	closesocket(sock);
//
//	CloseHandle(fd);
//
//	CommClient::CleanEnvironment();
//
	system("pause");
}
