#include "svpn_fd.h"
#include "svpn_server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>

// #define DEV_NAME_LEN	128

int svpn_tun_create(char *dev_name)
{
	fd_t fd;
	int err;
//	char dev[] = "/dev/net/tun";
	struct ifreq ifr;

	if((fd = open(dev, O_RDWR)) < 0)
	{
		perror("tun_create");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if ((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0)
	{
		close(fd);
		return err;
	}
	return fd;
}

int svpn_socket_create(struct svpn_server *psc, unsigned short port)
{
	int n = 1024 * 1024;
	socklen_t slen = sizeof(n);
	psc->server_addr.sin_family = AF_INET;
	psc->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	psc->server_addr.sin_port = htons(port);
	psc->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(setsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, slen) == -1) {
		printf("???\n");
	}
	if(getsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, &slen) == -1) {
		printf("wrong arg?\n");
	}
	printf("n=%d\n", n);
	if(psc->sock_fd < 0) {
		perror("socket");
		return -1;
	}
	if(bind(psc->sock_fd, (struct sockaddr*)&psc->server_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		close(psc->sock_fd);
		return -1;
	}
	return 0;
}
