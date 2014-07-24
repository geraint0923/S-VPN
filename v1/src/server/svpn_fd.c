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

#define DEV_NAME_LEN	128

static int svpn_set_tuntxqlen(char *name, int qlen) {
	struct ifreq netifr;
	int ctl_fd;

	if((ctl_fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		strcpy(netifr.ifr_name, name);
		netifr.ifr_qlen = qlen;
		if(ioctl(ctl_fd, SIOCSIFTXQLEN, (void*)&netifr) < 0) {
			perror("ioctl SIOCSIFTXQLEN");
		}
		close(ctl_fd);
	}
	return 0;
}

int svpn_tun_create(char *dev_name, int laddr)
{
	int fd, err;
	const char dev[] = "/dev/net/tun";
	struct ifreq ifr;
	struct in_addr lcaddr;
	char cmd[128];

	lcaddr.s_addr = laddr;

	if((fd = open(dev, O_RDWR)) < 0) {
		perror("tun_create");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI | IFF_ONE_QUEUE;

	if((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0) {
		close(fd);
		return err;
	}

	strcpy(dev_name, ifr.ifr_name);
	svpn_set_tuntxqlen(dev_name, 1000);

	sprintf(cmd, "ifconfig %s up", dev_name);
	if ((err = system(cmd)) != 0)
		return err;
	printf("[Info] %s setup.\n", dev_name);

	sprintf(cmd, "ifconfig %s mtu 1400", dev_name);
	if ((err = system(cmd)) != 0)
		return err;
	printf("[Info] %s mtu = 1400\n", dev_name);

	sprintf(cmd, "ifconfig %s %s/24", dev_name, inet_ntoa(lcaddr));
	if ((err = system(cmd)) != 0)
		return err;
	printf("[Info] Tunnel ipaddress %s\n", inet_ntoa(lcaddr));

	return fd;
}

int svpn_sock_create(struct svpn_server *psc, unsigned short port)
{
	int n;
	socklen_t slen = sizeof(n);

	psc->server_addr.sin6_family = PF_INET6;
	psc->server_addr.sin6_addr = in6addr_any;
	psc->server_addr.sin6_port = htons(port);
	psc->sock_fd = socket(PF_INET6, SOCK_DGRAM, 0);

	n = 1024 * 1024;
	if (setsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, slen) == -1)
		fprintf(stderr, "[Error] Receive buffer failed to set.\n");

	n = 1024 * 8;
	if(setsockopt(psc->sock_fd, SOL_SOCKET, SO_SNDBUF, &n, slen) == -1)
		fprintf(stderr, "[Error] Send buffer size set failed.\n");

	printf("[Info] Sendbuffer = %d", n);
	if(getsockopt(psc->sock_fd, SOL_SOCKET, SO_RCVBUF, &n, &slen) == -1)
		fprintf(stderr, "[Error] Get recvbuffer size failed.\n");
	printf(", Recvbuffer = %d\n", n);

	if(psc->sock_fd < 0)
	{
		perror("[Fatal] Socket ");
		return -1;
	}

	if(bind(psc->sock_fd, (struct sockaddr*)&psc->server_addr,
				sizeof(struct sockaddr_in6)) < 0)
	{
		perror("\033[0;40;32m[Fatal] Socket Bind ");
		close(psc->sock_fd);
		return -1;
	}
	return 0;
}
