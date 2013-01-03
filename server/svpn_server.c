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

#include "svpn_server.h"
#include "crypt.h"
#include "svpn_fd.h"
#include "svpn_net.h"

#define BUFFER_LEN	4096

static void svpn_sig_handler(int sig) {
	char buffer[] = "Signal?\n";
	write(1, buffer, strlen(buffer));
}

static int svpn_server_match(struct svpn_server *psc, struct sockaddr_in *addr) {
	int i;
	if(!psc) {
		return -1;
	}

	for(i = 2; i < psc->client_count; ++i) {
		if(psc->node_list[i]) {
			if(psc->node_list[i]->node_addr.sin_addr.s_addr == addr->sin_addr.s_addr) {
				return i;
			}
		}
	}

	return -2;
}

static void *svpn_server_recv_thread(void *pvoid) {
	printf("ok************\n");

	struct svpn_server *psc = (struct svpn_server*)pvoid;

	struct sockaddr_in addr;
	socklen_t alen = sizeof(addr);

	unsigned char buffer[BUFFER_LEN], tmp_buffer[BUFFER_LEN];
	int idx;
//	pthread_detach(pthread_self());

	printf("recv_loop\n");

	while(psc->recv_thread_on) {
		int len = recvfrom(psc->sock_fd, tmp_buffer, BUFFER_LEN, 0, 
				(struct sockaddr*)&addr, &alen);
		printf("recv : %d\n", len);
		if(len < 0) {
			if(errno == EINTR) {
				printf("EINTR ! exit.\n");
				return NULL;
			}
			printf("nothing ?\n");
			continue;
		}

		idx = svpn_server_match(psc, &addr);
		if(idx < 0) {
			continue;
		}


		Decrypt(&(psc->node_list[idx]->table), tmp_buffer, buffer, len);

		len = write(psc->tun_fd, buffer, len);
	}
	
	return NULL;
}

static void *svpn_server_send_thread(void *pvoid) {

	struct svpn_server *psc = (struct svpn_server*)pvoid;
//	pthread_detach(pthread_self());
	struct svpn_net_ipv4_header *pd = NULL;
	unsigned char buffer[BUFFER_LEN], tmp_buffer[BUFFER_LEN];
	int idx;

//	pthread_detach(pthread_self());
	printf("send_loop\n");

	while(psc->send_thread_on) {
		int len = read(psc->tun_fd, tmp_buffer, BUFFER_LEN);

		printf("send : %d\n", len);
		if(len < 0) {
			if(errno == EINTR) {
				printf("EINTR in send ! exit.\n");
				return NULL;
			}
			printf("nothing in send\n");
			continue;
		}

		// do something to find the node and send to a right place
		pd = (struct svpn_net_ipv4_header*) buffer;
		idx = pd->dst_ip[4];
		if(!psc->node_list[idx]) {
			continue;
		}
		

		Encrypt(&(psc->node_list[idx]->table), tmp_buffer, buffer, len);

		len = sendto(psc->sock_fd, buffer, len, 0,
				(struct sockaddr*)&(psc->node_list[idx]->node_addr), sizeof(psc->server_addr));
	}

	return NULL;
}

struct svpn_server *svpn_server_init(unsigned short port, int c_count) {

	struct svpn_server *psc = (struct svpn_server*) malloc(sizeof(struct svpn_server));
	struct sigaction sact;
	int bytes = 0;
	memset(&sact, 0, sizeof(struct sigaction));
	memset(psc, 0, sizeof(struct svpn_server));

//	BuildTable(&(psc->table), md5, timestamp);


	// init socket
	if(svpn_sock_create(psc, port) < 0) {
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

	psc->client_count = c_count;
	bytes = sizeof(struct svpn_client_node*) * c_count;
	psc->node_list = (struct svpn_client_node**) malloc(bytes);
	memset(psc->node_list, 0, bytes);

	//signal(SIGUSR1, svpn_sig_handler);
	sact.sa_handler = svpn_sig_handler;
	sact.sa_flags &= ~SA_RESTART;
	sigaction(SIGUSR1, &sact, &(psc->old_act));

out:
	return psc;
}

int svpn_server_release(struct svpn_server *psc) {
	int i;
	if(!psc) {
		return -1;
	}

	if(psc->recv_thread_on) {
		svpn_server_stop_recv_thread(psc);
	}
	if(psc->send_thread_on) {
		svpn_server_stop_send_thread(psc);
	}

	close(psc->tun_fd);
	close(psc->sock_fd);

	sigaction(SIGUSR1, &(psc->old_act), NULL);

	for(i = 0; i < psc->client_count; ++i) {
		svpn_server_remove_client(psc, i);
	}

	if(psc->node_list) {
		free(psc->node_list);
	}

	free(psc);
	
	return 0;
}

int svpn_server_start_recv_thread(struct svpn_server *psc) {
	if(!psc) {
		return -1;
	}
//	psc->recv_thread_on = 1;
	//printf("start recv : 0x%08x\n", psc);
	psc->recv_thread_on = 1;
	//printf("he\nsdasdasdas*******\n");
	pthread_create(&(psc->recv_tid), NULL, &svpn_server_recv_thread, (void*)psc);
	printf("he man\n");
	return 0;
}

int svpn_server_start_send_thread(struct svpn_server *psc) {
	if(!psc) {
		return -1;
	}
	printf("start send\n");
	psc->send_thread_on = 1;
	pthread_create(&(psc->send_tid), NULL, &svpn_server_send_thread, (void*)psc);
	return 0;
}

int svpn_server_stop_recv_thread(struct svpn_server *psc) {
	if(!psc) {
		return -1;
	}
	psc->recv_thread_on = 0;
	// to disturb the thread?
	pthread_kill(psc->recv_tid, SIGUSR1);
	return 0;
}

int svpn_server_stop_send_thread(struct svpn_server *psc) {
	if(!psc) {
		return -1;
	}
	psc->send_thread_on = 0;
	// to disturb the thread?
	pthread_kill(psc->send_tid, SIGUSR1);
	return 0;
}

int svpn_server_wait_recv_thread(struct svpn_server *psc) {
	void *value;
	if(!psc) {
		return -1;
	}
	return pthread_join(psc->recv_tid, &value);
}

int svpn_server_wait_send_thread(struct svpn_server *psc) {
	void *value;
	if(!psc) {
		return -1;
	}
	return pthread_join(psc->send_tid, &value);
}

int svpn_server_available_index(struct svpn_server *psc) {
	int i;
	if(!psc) {
		return -1;
	}
	for(i = 2; i < psc->client_count; ++i) {
		if(!psc->node_list[i]) {
			return i;
		}
	}
	return -1;
}

int svpn_server_remove_client(struct svpn_server *psc, int idx) {
	if(!psc) {
		return -1;
	}
	if(psc->node_list[idx]) {
		free(psc->node_list[idx]);
		psc->node_list[idx] = NULL;
	}
}

int svpn_server_add_client(struct svpn_server *psc, int idx, char *addr,
		unsigned char *pmd5, long long timestamp) {
	if(!psc) {
		return -1;
	}

	if(psc->node_list[idx]) {
		return -2;
	}

	psc->node_list[idx] = (struct svpn_client_node*)malloc(sizeof(struct svpn_client_node));
	memset(psc->node_list[idx], 0, sizeof(struct svpn_client_node));

	BuildTable(&(psc->node_list[idx]->table), pmd5, timestamp);
	psc->node_list[idx]->node_addr.sin_addr.s_addr = inet_addr(addr);
	psc->node_list[idx]->node_addr.sin_family = AF_INET;
	psc->node_list[idx]->node_addr.sin_port = htons(33333);

	return 0;
}
