#include "svpn_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "md5.h"
#include "crypt.h"
#include "svpn_fd.h"

struct svpn_client *svpn_init(char *addr, unsigned short port,
		unsigned char *md5, long long timestamp) {

	struct svpn_client *psc = (struct svpn_client*) malloc(sizeof(struct svpn_client));
	memset(psc, 0, sizeof(struct svpn_client));
	BuildTable(&(psc->table), md5, timestamp);


	// init socket
	if(svpn_sock_create(psc, addr, port) < 0) {
		free(psc);
		psc = NULL;
		goto out;
	}


	// init tunnel
	psc->tun_fd = svpn_tun_create(psc->dev_name);
	if(psc->tun_fd < 0) {
		close(psc->sock_fd);
		free(psc);
		psc = NULL;
		goto out;
	}

out:
	return psc;
}

int svpn_release(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}

	if(psc->recv_thread_on) {
		svpn_stop_recv_thread(struct svpn_client *psc);
	}
	if(psc->send_thread_on) {
		svpn_stop_send_thread(struct svpn_client *psc);
	}

	close(psc->tun_fd);
	close(psc->sock_fd);

	free(psc);
}

int svpn_start_recv_thread(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}
	return 0;
}

int svpn_start_send_thread(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}
	return 0;
}

int svpn_stop_recv_thread(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}
	return 0;
}

int svpn_stop_send_thread(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}
	return 0;
}


