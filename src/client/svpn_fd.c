#include "svpn_fd.h"
#include "svpn_client.h"
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

#define DEV_NAME_LEN	128

int svpn_tun_create(char *dev_name) {
	int fd, err;
	char dev[] = "/dev/net/tun";
	struct ifreq ifr;

	if((fd = open(dev, O_RDWR)) < 0) {
		perror("tun_create");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0) {
		close(fd);
		return err;
	}

	strcpy(dev_name, dev);
	return fd;
}



int svpn_sock_create(struct svpn_client *psc,
		char *addr, unsigned short port) {
	int n = 8 * 1024;
	socklen_t slen = sizeof(n);
	struct sockaddr_in haddr;
	memset(&haddr, 0, sizeof(addr));
	haddr.sin_family = PF_INET;
	haddr.sin_addr.s_addr = INADDR_ANY;
	haddr.sin_port = htons(123);

	psc->server_addr.sin_family = PF_INET;
//	psc->server_addr.sin6_addr = inet_addr(addr);
	inet_pton(PF_INET, addr, &psc->server_addr.sin_addr);
	psc->server_addr.sin_port = htons(port);
	psc->sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(psc->sock_fd < 0) {
		perror("socket");
		return -1;
	}
	/*
	if(fcntl(psc->sock_fd, F_SETFL, O_NONBLOCK) != 0) {
		printf("fcntl failed\n");
	}
	*/

	/*
	if(setsockopt(psc->sock_fd, SOL_SOCKET, SO_SNDBUF, &n, slen) == -1) { 
		printf("set snd_buf failed\n");
	}
	*/
	n = 1024 * 1024;
	if(setsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, slen) == -1) { 
		printf("set recv_buf failed\n");
	}
	if(getsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, &slen) == -1) {
		printf("recv_buf not get\n");
	}

	printf("recv_buf:%d\n", n);
	if(bind(psc->sock_fd, (struct sockaddr*)&haddr, sizeof(haddr)) < 0) {
		perror("bind");
		return -2;
	}
	return 0;
}

