#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>

#include "svpn_client.h"
#include "crypt.h"
#include "svpn_fd.h"

#define BUFFER_LEN	4096

static void svpn_sig_handler(int sig) {
	char buffer[] = "Signal?\n";
	write(1, buffer, strlen(buffer));
}

int svpn_handle_thread(struct svpn_client* pvoid) {
	struct svpn_client *psc = pvoid;

	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);
	unsigned char buffer[BUFFER_LEN], tmp_buffer[BUFFER_LEN];
//	struct timeval tv;
	fd_set fd_list;
	int maxfd = (psc->sock_fd > psc->tun_fd) ? psc->sock_fd : psc->tun_fd;
	int ret, len;
	int recvc = 0, sendc = 0;

	maxfd++;
//	tv.tv_sec = 0;
//	tv.tv_usec = 0;
	while(1) {
		FD_ZERO(&fd_list);
		FD_SET(psc->tun_fd, &fd_list);
		FD_SET(psc->sock_fd, &fd_list);

		ret = select(maxfd, &fd_list, NULL, NULL, NULL);
		if(ret < 0) {
			if(errno == EINTR)
				return 0;
			continue;
		}

		if(FD_ISSET(psc->tun_fd, &fd_list)) {
			len = read(psc->tun_fd, tmp_buffer, BUFFER_LEN);
			
			if (len < 0 || len > BUFFER_LEN)
				continue;

			sendc += len;

//			printf("send : %d total:%d\n", len, sendc);

			Encrypt(&(psc->table), tmp_buffer, buffer, len);

//			len = sendto(psc->sock_fd, buffer, len, 0,
//`					(struct sockaddr*)&(psc->server_addr), sizeof(psc->server_addr));

			len = sendto(psc->sock_fd, buffer, len, 0,
					(struct sockaddr*)&(psc->server_addr), sizeof(psc->server_addr));



			if(len <= 0) {
				printf("non-blocked, drop the packet\n");
			}
	
		}

		if(FD_ISSET(psc->sock_fd, &fd_list)) {
			len = recvfrom(psc->sock_fd, tmp_buffer, BUFFER_LEN, 0,
					(struct sockaddr*)&addr, &alen);

			if (len < 0 || len > BUFFER_LEN)
				continue;

			recvc += len;

//			printf("recv : %d total:%d\n", len, recvc);

			Decrypt(&(psc->table), tmp_buffer, buffer, len);

			if (buffer[0] >> 4 != 4)
				continue;

			len = write(psc->tun_fd, buffer, len);
		}

	}
	return 0;
}

struct svpn_client *svpn_init(char *addr, unsigned short port,
		unsigned char *md5, long long timestamp) {

	struct svpn_client *psc = (struct svpn_client*) malloc(sizeof(struct svpn_client));
	struct sigaction sact;
	memset(&sact, 0, sizeof(struct sigaction));
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


	//signal(SIGUSR1, svpn_sig_handler);
	sact.sa_handler = svpn_sig_handler;
	sact.sa_flags &= ~SA_RESTART;
	sigaction(SIGUSR1, &sact, &(psc->old_act));

out:
	return psc;
}

/*
int svpn_release(struct svpn_client *psc) {
	if(!psc) {
		return -1;
	}

	if(psc->recv_thread_on) {
		svpn_stop_recv_thread(psc);
	}
	if(psc->send_thread_on) {
		svpn_stop_send_thread(psc);
	}

	close(psc->tun_fd);
	close(psc->sock_fd);

	sigaction(SIGUSR1, &(psc->old_act), NULL);

	free(psc);
	
	return 0;
}
*/

