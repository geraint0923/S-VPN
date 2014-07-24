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
#include <net/if.h>
#include <net/if_utun.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <sys/ioctl.h>

#define DEV_NAME_LEN	128

int svpn_tun_create(char *dev_name) {
	struct sockaddr_ctl sc;
	struct ctl_info ctlInfo;
	int s;

	memset(&ctlInfo, 0, sizeof(ctlInfo));
	strncpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name));
	
	s = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
	if(s < 0) {
		perror("socket");
		return -1;
	}

	if(ioctl(s, CTLIOCGINFO, &ctlInfo) == -1) {
		perror("CTLIOCGINFO");
		close(s);
		return -1;
	}
	sc.sc_family = PF_SYSTEM;
	sc.ss_sysaddr = AF_SYS_CONTROL;
	sc.sc_id = ctlInfo.ctl_id;
	sc.sc_len = sizeof(sc);

	sc.sc_unit = 0;
	if(connect(s, (struct sockaddr*)&sc, sizeof(sc)) == -1) {
		perror("connect");
		close(s);
		return -1;
	}
	return s;
}

int svpn_sock_create(struct svpn_client *psc,
		char *addr, unsigned short port) {
	int n = 1024 * 1024;
	socklen_t slen = sizeof(n);
	struct sockaddr_in haddr;
	memset(&haddr, 0, sizeof(addr));
	haddr.sin_family = AF_INET;
	haddr.sin_addr.s_addr = htonl(INADDR_ANY);
	haddr.sin_port = htons(33333);

	psc->server_addr.sin_family = AF_INET;
	psc->server_addr.sin_addr.s_addr = inet_addr(addr);
	psc->server_addr.sin_port = htons(port);
	psc->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(psc->sock_fd < 0) {
		perror("socket");
		return -1;
	}
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
