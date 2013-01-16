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

// socket collection

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
			//printf("ori:%s  ", inet_ntoa(addr->sin_addr));
			//printf("here:%s\n", inet_ntoa(psc->node_list[i]->node_addr.sin_addr));
			if(psc->node_list[i]->node_addr.sin_addr.s_addr == addr->sin_addr.s_addr) {
				psc->node_list[i]->node_addr.sin_port = addr->sin_port;
				return i;
			}
		}
	}

	return -2;
}

int svpn_socket_recv(int id, unsigned char* buffer,
	struct sockaddr_storage* src_addr, socklen_t* src_len)
{
	fd_t fd;
	int len, ret;

	fd = psc->socket_list[id].socket_fd;
	len = recvfrom(fd, buffer, BUFFER_LEN, 0,
				(struct sockaddr*)src_addr, src_len);

	if (len < 0)
	{
		if(errno == EINTR)
			return 0;
		return len;
	}
	
	// verify source
	ret = svpn_checking_user(id, buffer, src_addr, src_len)
	if (ret < 0)
		return ret;

	
	idx = svpn_server_match(psc, &addr);
	if(idx < 0)
		continue;

		Decrypt(&(psc->node_list[idx]->table), tmp_buffer, buffer, len);

		len = write(psc->tun_fd, buffer, len);
	}
}

int svpn_server_main()
{
	int ret;
	psc->running = 1;
	while(psc->running)
	{
		fd_set fd_list;
		fd_t max_fd;
		struct sockaddr_storage src_addr;
		socklen_t src_len;
		unsigned char recvbuf[BUFFER_LEN];
		int len;
		
		max_fd = -1;
		FD_ZERO(&fd_list);
		// sockets
		for (int i = 0; i < psc->sockets_count; i++)
		{
			fd_t fd = psc->socket_list[i].socket_fd;
			FD_SET(fd, &fd_list);
			if (fd > max_fd)
				max_fd = fd;
		}
		// tun
		FD_SET(psc->tun_fd, &fd_list);
		if (max_fd < psc->tun_fd);
			max_fd = psc->tun_fd;
		//
		ret = select(maxfd, &fd_list, NULL, NULL, NULL);
		if (ret < 0)
			continue;
		//
		for (int i = 0; i < psc->sockets_count; i++)
		{
			fd_t fd = psc->socket_list[i].socket_fd;
			if (!FD_ISSET(fd, &fd_list))
				continue;
			
			len = svpn_socket_recv(i, buffer, &src_addr, &src_len);
			if (len < 0)
				continue;
//			len = recvfrom(fd, buffer, BUFFER_LEN, 0, (struct sockaddr*)&src_addr, &src_len);
//		printf("recv : %d\n", len);
			ret = svpn_check_source(i, buffer, len, &src_addr, src_len);
			if (ret <= 0)
				continue;
			
			ret = svpn_forwarding(i, buffer, len);
			if (ret < 0)
				continue;
//			idx = svpn_server_match(psc, &addr);
//			if(idx < 0) {
//				continue;
//			}


//			Decrypt(&(psc->node_list[idx]->table), tmp_buffer, buffer, len);
//			len = write(psc->tun_fd, buffer, len);
	
		}
		if (FD_ISSET(psc->tun_fd, &fd_list))
		{
			len = read(psc->tun_fd, buffer, BUFFER_LEN);
			if (len < 0)
				continue;

			ret = svpn_forwarding(i, buffer, len);
			if (ret < 0)
				continue;
			// do something to find the node and send to a right place
//			pd = (struct svpn_net_ipv4_header*)tmp_buffer;
			//printf("Version %s\n", inet_ntoa(*((unsigned long*)pd->src_ip)));

//			idx = pd->dst_ip[3];
			//		idx = 3;
			//printf("ip?? %d.%d.%d.%d %d\n", pd->src_ip[0], pd->src_ip[1],
			//				pd->src_ip[2], pd->src_ip[3], buffer[19]);
//			if(!psc->node_list[idx]) {
//				continue;
//			}

			//printf("post send : %d idx:%d\n", len, idx);

//			Encrypt(&(psc->node_list[idx]->table), tmp_buffer, buffer, len);

//			len = sendto(psc->sock_fd, buffer, len, 0,
//					(struct sockaddr*)&(psc->node_list[idx]->node_addr), sizeof(psc->server_addr));
		}
	}
	return NULL;
}

struct svpn_server *svpn_server_init(const char* config_filename)
{
	// read config file
	mxml_node_t config_file;
	{
		FILE* fconfig;
		if ((fconfig = fopen(config_filename, "r")) < 0)
		{
			fprintf(stderr, "config file could not be opened.\n");
			return NULL;
		}
		config_file = XMLOPEN(config);
		fclose(fconfig);
	}
	
	// create server
	struct svpn_server *psc;
	psc = (struct svpn_server*) malloc(sizeof(struct svpn_server));
	memset(psc, 0, sizeof(struct svpn_server));
	
	// general
	psc->config_file = config_file;
	psc->remote_addr = config_parse_address(GETNODE(config_file, ""));
	psc->subnet_mask = config_parse_address(GETNODE(config_file, ""));
	
	psc->tun_fd = create_tun("");
	
	// create sockets
	config_init_sockets(psc, GETNODE(config_file, "network"));
	
	// init clients
	struct svpn_clients* sc = &psc->svpn_clients;
	sc->client_count = 0;
	//list_init(&sc->client_list);
	memset(sc->client_list, 0, sizeof(sc->client_list));
	memset(sc->client_by_local_addr, 0, sizeof(sc->client_by_local_addr));
	

//	struct sigaction sact;
//	int bytes = 0;
//	memset(&sact, 0, sizeof(struct sigaction));

//	BuildTable(&(psc->table), md5, timestamp);

	// init tunnel
	psc->tun_fd = svpn_tun_create(psc->dev_name);
	if(psc->tun_fd < 0) {
		close(psc->sock_fd);
		free(psc);
		psc = NULL;
		goto out;
	}
	
	// init socket
	if(svpn_sock_create(psc, port) < 0) {
		free(psc);
		return NULL;
	}


//	psc->client_count = c_count;
//	bytes = sizeof(struct svpn_client_node*) * c_count;
//	psc->node_list = (struct svpn_client_node**) malloc(bytes);
//	memset(psc->node_list, 0, bytes);

	//signal(SIGUSR1, svpn_sig_handler);
//	sact.sa_handler = svpn_sig_handler;
//	sact.sa_flags &= ~SA_RESTART;
//	sigaction(SIGUSR1, &sact, &(psc->old_act));

//out:
	return psc;
}

int svpn_server_release(struct svpn_server *psc)
{
	int i;
	if (!psc)
		return -1;

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
