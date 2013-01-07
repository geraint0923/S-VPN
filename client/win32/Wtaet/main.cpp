#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "config.h"
#include "Client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")



int main(int argc, char** argv)
{
	extern int main2(int argc, char *argv[]);
	main2(0, NULL);


	PMIB_IPFORWARDTABLE pIpForwardTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    char szDestIp[128];
    char szMaskIp[128];
    char szGatewayIp[128];

    struct in_addr IpAddr;

    int i;

    pIpForwardTable =
        (MIB_IPFORWARDTABLE *)malloc(sizeof (MIB_IPFORWARDTABLE));
    if (pIpForwardTable == NULL) {
        printf("Error allocating memory\n");
        return 1;
    }

    if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) ==
        ERROR_INSUFFICIENT_BUFFER) {
        free(pIpForwardTable);
        pIpForwardTable = (MIB_IPFORWARDTABLE *) malloc(dwSize);
        if (pIpForwardTable == NULL) {
            printf("Error allocating memory\n");
            return 1;
        }
    }

    /* Note that the IPv4 addresses returned in 
     * GetIpForwardTable entries are in network byte order 
     */
    if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0)) == NO_ERROR) {
        printf("\tNumber of entries: %d\n",
               (int) pIpForwardTable->dwNumEntries);
        for (i = 0; i < (int) pIpForwardTable->dwNumEntries; i++) {
            /* Convert IPv4 addresses to strings */
            IpAddr.S_un.S_addr =
                (u_long) pIpForwardTable->table[i].dwForwardDest;
            strcpy_s(szDestIp, sizeof (szDestIp), inet_ntoa(IpAddr));
            IpAddr.S_un.S_addr =
                (u_long) pIpForwardTable->table[i].dwForwardMask;
            strcpy_s(szMaskIp, sizeof (szMaskIp), inet_ntoa(IpAddr));
            IpAddr.S_un.S_addr =
                (u_long) pIpForwardTable->table[i].dwForwardNextHop;
            strcpy_s(szGatewayIp, sizeof (szGatewayIp), inet_ntoa(IpAddr));

            printf("\n\tRoute[%d] Dest IP: %s\n", i, szDestIp);
            printf("\tRoute[%d] Subnet Mask: %s\n", i, szMaskIp);
            printf("\tRoute[%d] Next Hop: %s\n", i, szGatewayIp);
            printf("\tRoute[%d] If Index: %ld\n", i,
                   pIpForwardTable->table[i].dwForwardIfIndex);
            printf("\tRoute[%d] Type: %ld - ", i,
                   pIpForwardTable->table[i].dwForwardType);
            switch (pIpForwardTable->table[i].dwForwardType) {
            case MIB_IPROUTE_TYPE_OTHER:
                printf("other\n");
                break;
            case MIB_IPROUTE_TYPE_INVALID:
                printf("invalid route\n");
                break;
            case MIB_IPROUTE_TYPE_DIRECT:
                printf("local route where next hop is final destination\n");
                break;
            case MIB_IPROUTE_TYPE_INDIRECT:
                printf
                    ("remote route where next hop is not final destination\n");
                break;
            default:
                printf("UNKNOWN Type value\n");
                break;
            }
            printf("\tRoute[%d] Proto: %ld - ", i,
                   pIpForwardTable->table[i].dwForwardProto);
            switch (pIpForwardTable->table[i].dwForwardProto) {
            case MIB_IPPROTO_OTHER:
                printf("other\n");
                break;
            case MIB_IPPROTO_LOCAL:
                printf("local interface\n");
                break;
            case MIB_IPPROTO_NETMGMT:
                printf("static route set through network management \n");
                break;
            case MIB_IPPROTO_ICMP:
                printf("result of ICMP redirect\n");
                break;
            case MIB_IPPROTO_EGP:
                printf("Exterior Gateway Protocol (EGP)\n");
                break;
            case MIB_IPPROTO_GGP:
                printf("Gateway-to-Gateway Protocol (GGP)\n");
                break;
            case MIB_IPPROTO_HELLO:
                printf("Hello protocol\n");
                break;
            case MIB_IPPROTO_RIP:
                printf("Routing Information Protocol (RIP)\n");
                break;
            case MIB_IPPROTO_IS_IS:
                printf
                    ("Intermediate System-to-Intermediate System (IS-IS) protocol\n");
                break;
            case MIB_IPPROTO_ES_IS:
                printf("End System-to-Intermediate System (ES-IS) protocol\n");
                break;
            case MIB_IPPROTO_CISCO:
                printf("Cisco Interior Gateway Routing Protocol (IGRP)\n");
                break;
            case MIB_IPPROTO_BBN:
                printf("BBN Internet Gateway Protocol (IGP) using SPF\n");
                break;
            case MIB_IPPROTO_OSPF:
                printf("Open Shortest Path First (OSPF) protocol\n");
                break;
            case MIB_IPPROTO_BGP:
                printf("Border Gateway Protocol (BGP)\n");
                break;
            case MIB_IPPROTO_NT_AUTOSTATIC:
                printf("special Windows auto static route\n");
                break;
            case MIB_IPPROTO_NT_STATIC:
                printf("special Windows static route\n");
                break;
            case MIB_IPPROTO_NT_STATIC_NON_DOD:
                printf
                    ("special Windows static route not based on Internet standards\n");
                break;
            default:
                printf("UNKNOWN Proto value\n");
                break;
            }

            printf("\tRoute[%d] Age: %ld\n", i,
                   pIpForwardTable->table[i].dwForwardAge);
            printf("\tRoute[%d] Metric1: %ld\n", i,
                   pIpForwardTable->table[i].dwForwardMetric1);
        }
        free(pIpForwardTable);
     //   return 0;
    } else {
        printf("\tGetIpForwardTable failed.\n");
        free(pIpForwardTable);
        return 1;
    }


	//return 0;

	if (argc == 11)
	{
		printf("SVPN Windows Client, Version Alpha 0.1.0\n");
		printf("Copyright Yangyang, Huxiaoxiang\n");
		printf("\n");
		printf("Tap-Win32 Driver (MUST BE V9.9) is required.\n");
		printf("It can be installed with newest OpenVPN, or individually.\n");
		printf("\n");
		printf("Parameters:\n");
		printf("argv[1] : Tun IP address, e.g. 192.168.3.6\n");
		printf("argv[2] : Server name or IP, e.g. www.huxx.me\n");
		printf("argv[3] : Server UDP port, e.g. 33333\n");
		printf("Currently username and password is immutable.\n");
		return 0;
	}
	Config cfg;
//	cfg.UseCommandLineConfig(argv);
	Client cl;
	cl.Initialize(cfg);
	cl.Run();
	cl.Finalize();
}
