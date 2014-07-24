#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "crypt.h"
#include "md5.h"
#include <unistd.h>
#include <netinet/in.h>
#include "svpn_client.h"
#include <signal.h>

#define BUFFER_LEN	4096

int main(int argc, char **argv) {
	struct svpn_client *psc = NULL;
	char buffer[1024];

	if(argc < 5) {
		printf("need more parameters : %s server_ip port id password\n", argv[0]);
		return 0;
	}
	unsigned char md5pd[16];

	MD5Fast(argv[4], strlen(argv[4]), md5pd);

	psc = svpn_init(argv[1], atoi(argv[2]), md5pd, atoi(argv[3]));
	if(!psc) {
		printf("null pointer!\n");
		return -1;
	}

	sprintf(buffer, "ifconfig %s up", "tun0");
	system(buffer);
	sprintf(buffer, "ifconfig %s 192.168.3.%s/24", "tun0", argv[3]);
	system(buffer);
	sprintf(buffer, "ifconfig %s mtu 1400", "tun0");
	system(buffer);

	printf("client start\n");
	svpn_handle_thread(psc);

	return 0;
}

