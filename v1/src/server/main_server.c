#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "crypt.h"
#include "svpn_server.h"

int main(int argc, char** argv)
{
	int ret;
	struct svpn_server *psc = NULL;

	if (argc != 2)
	{
		fprintf(stderr, "[Fatal] Argument invalid.\n");
		return -1;
	}

	if ((psc = svpn_server_init(argv[1])) == NULL)
	{
		fprintf(stderr, "[Fatal] Failed to create server.\n");
		return -1;
	}

	if (svpn_server_init_client(psc, argv[1]) < 0)
	{
		fprintf(stderr, "[Fatal] Failed to initialze users.\n");
		return -1;
	}	

	if ((ret = nice(-19)) != 0)
	{
	//	fprintf(stderr, "[Warn] Nice set error. %d.\n", ret);
	}

//	system("ifconfig tun0 up");
//	system("ifconfig tun0 mtu 1400");
//	system("ifconfig tun0 192.168.3.1/24");

	svpn_server_handle_thread(psc);

	return 0;
}

