#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "server.h"
#include "crypt.h"
#include "client.h"
#include "fd.h"
#include "net.h"

using namespace std;

long long now;

void svpn_update_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	now = ((long long)tv.tv_sec) * 1000000 + tv.tv_usec;
}

int client_sendto(int client_id, const unsigned char* buffer, size_t len, int flag)
{
	if (((int)psc->clients.client_count) <= client_id)
		return -1;
	struct svpn_client* client = psc->clients.clients_list[client_id];
	return socket_sendto(client->socket_id, buffer, len, flag,
			&client->node_addr, client->node_addr_len, client_id);
}

int handle_control_package(int sock_id, const unsigned char* buffer,
							int flag,
							const struct sockaddr_storage* src_addr,
							socklen_t src_len)
{
	int ret;
	const struct control_package* ctl_pkg = (const struct control_package*)buffer;
	// check control package
	if (!(flag & PACKAGE_CONTROL))
		return -ERROR_INVALID_PACKAGE;
	switch (ctl_pkg->type)
	{
	case PACKAGE_CONTROL_USERNAME:
		{
			int uid;
			// add to candidate, ret = uid
			uid = ret = cache_client(sock_id, ctl_pkg->content.pkg_username.username,
						src_addr, src_len);
			// send replay
			struct control_package reply;
			reply.type = PACKAGE_CONTROL_TIMESTAMP;
			if (ret < 0)
			{
				reply.content.pkg_timestamp.timestamp = 0LL;
				socket_sendto(sock_id, (const unsigned char*)&reply,
					PACKAGE_UNENCRYPTED | PACKAGE_CONTROL,
					PACKAGE_SIZE_CONTROL_TIMESTAMP, src_addr, src_len, uid);
				return ret;
			}
			else
			{
				reply.content.pkg_timestamp.timestamp = now;
				client_sendto(ret, (const unsigned char*)&reply,
					PACKAGE_ENCRYPTED | PACKAGE_CONTROL,
					PACKAGE_SIZE_CONTROL_TIMESTAMP);
				return 0;
			}
		}
		break;
	case PACKAGE_CONTROL_KEEPALIVE:
		{
			int uid;
			struct control_package reply;
			reply.type = PACKAGE_CONTROL_LOGIN;
			if (ctl_pkg->content.pkg_keepalive.magic_number !=
				SVPN_MAGIC_NUMBER)
			{
				reply.content.pkg_login.status = SVPN_WRONG_PASSWORD;
				ret = client_sendto(ret, (const unsigned char*)&reply,
					PACKAGE_ENCRYPTED | PACKAGE_CONTROL,
					PACKAGE_SIZE_CONTROL_LOGIN);
				return ret;
			}
			// TODO: check options. e.g. ipv6
			// add to candidate, ret = uid
			uid = ret = find_client_by_remote_addr(sock_id, src_addr, src_len);
			if (ret < 0)
				return 0;
				
			// send replay
			reply.content.pkg_login.status = SVPN_OK;
			reply.content.pkg_login.remote_address = psc->remote_addr;
			reply.content.pkg_login.network_mask = psc->subnet_mask;
			reply.content.pkg_login.local_address = get_unused_local_address();
			reply.content.pkg_login.valid_time = psc->clients.valid_time;
			client_sendto(uid, (const unsigned char*)&reply,
				PACKAGE_ENCRYPTED | PACKAGE_CONTROL,
				PACKAGE_SIZE_CONTROL_TIMESTAMP);

			ret = activate_client(uid, reply.content.pkg_login.local_address);
			if (ret < 0)
				printf("Error. But not fixedaste .\n"); // FIXME: error not handled here
			return 0;
		}
		break;
	default:
		return -ERROR_INVALID_PACKAGE;
		break;
	}
	return ret;
}

int svpn_forwarding(int sock_id, const unsigned char* buf, size_t len, int flag)
{
	// FIXME:TODO: check source & destination permission
	// IP package
	if (buf[0] >> 4 == 4) // IPv4
	{
		struct net_ipv4_header* header = (struct net_ipv4_header*)buf;
		if (((header->dst_ip & psc->subnet_mask) == (psc->remote_addr & psc->subnet_mask)) &&
				header->dst_ip != psc->remote_addr) // sub network
		{
			int client_id = find_client_by_local_addr(header->dst_ip);
			if (client_id < 0)
				return -1;
			return client_sendto(client_id, buf, len,
					PACKAGE_ENCRYPTED | PACKAGE_COMPRESSED);
		}
		else
			return tun_send(buf, len, 0);
	}
	if (buf[0] >> 4 == 6) // IPv6
	{
		printf("IPv6 not supported rightnow");
		return 0;
	}
	printf("unknown package?\n");
	return -1;
}

int svpn_server_main()
{
	int ret;
	psc->running = 1;
	while(psc->running)
	{
		int i;
		fd_set fd_list;
		fd_t max_fd;
		struct sockaddr_storage src_addr;
		socklen_t src_len;
		unsigned char buf[BUFFER_LENGTH];
		size_t len;
		int flag;
		
		max_fd = -1;
		FD_ZERO(&fd_list);
		// sockets
		for (i = 0; i < psc->sockets.socket_count; i++)
		{
			fd_t fd = psc->sockets.sockets_list[i].socket_fd;
			FD_SET(fd, &fd_list);
			if (fd > max_fd)
				max_fd = fd;
		}
		// tun
		FD_SET(psc->tun.tun_fd, &fd_list);
		if (max_fd < psc->tun.tun_fd)
			max_fd = psc->tun.tun_fd;
		//
		ret = select(max_fd +1, &fd_list, NULL, NULL, NULL);
		if (ret < 0)
			continue;
		svpn_update_time();
		//
		for (int i = 0; i < psc->sockets.socket_count; i++)
		{
			fd_t fd = psc->sockets.sockets_list[i].socket_fd;
			if (!FD_ISSET(fd, &fd_list))
				continue;
			
			ret = socket_recvfrom(i, buf, &len, &flag, &src_addr, &src_len);
			if (ret < 0)
				continue;
				
			if (buf[0] & PACKAGE_CONTROL)
				ret = handle_control_package(i, buf, flag, &src_addr, src_len);
			else
			{
				// TODO: check source
				// ret = svpn_check_source(i, buffer, len, &src_addr, src_len);
				//	if (ret <= 0)
				//		continue;
			
				ret = svpn_forwarding(i, buf, len, flag);
			}
		}
		if (FD_ISSET(psc->tun.tun_fd, &fd_list))
		{
			ret = tun_recv(buf, &len, &flag);
			if (ret < 0 || len <= 0)
				continue;

			ret = svpn_forwarding(i, buf, len, flag);
		}
	}
	return 0;
}

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

int svpn_server_init(const rapidxml::xml_node<>* config)
{
	int ret;
	// create server
	psc = (struct svpn_server*) malloc(sizeof(struct svpn_server));
	memset(psc, 0, sizeof(struct svpn_server));
	
	// general
	psc->config = config;
	psc->remote_addr = inet_addr(config->first_node("remoteaddr")->value());
	psc->subnet_mask = inet_addr(config->first_node("networkmask")->value());
	// tun
	if (config->first_node("tun") != NULL)
		psc->tun.tun_fd = svpn_tun_create(config->first_node("tun")->value());
	else
		psc->tun.tun_fd = svpn_tun_create(NULL);
	
	if (psc->tun.tun_fd < 0)
		return -1;

	//
	if ((ret = svpn_sockets_init()) < 0)
		return ret;
	if ((ret = svpn_clients_init()) < 0)
		return ret;

	return 0;
}

int svpn_server_release()
{
//	int i;
	if (psc == NULL)
		return -1;

	close(psc->tun.tun_fd);

//	close(psc->sock_fd);
//
//	sigaction(SIGUSR1, &(psc->old_act), NULL);
//
//	for(i = 0; i < psc->client_count; ++i) {
//		svpn_server_remove_client(psc, i);
//	}
//
//	if(psc->node_list) {
//		free(psc->node_list);
//	}

	free(psc);
	
	return 0;
}


//int svpn_server_add_client(struct svpn_server *psc, int idx, char *addr,
//		unsigned char *pmd5, long long timestamp) {
//	if(!psc) {
//		return -1;
//	}
//
//	if(psc->node_list[idx]) {
//		return -2;
//	}
//
//	psc->node_list[idx] = (struct svpn_client_node*)malloc(sizeof(struct svpn_client_node));
//	memset(psc->node_list[idx], 0, sizeof(struct svpn_client_node));
//
//	BuildTable(&(psc->node_list[idx]->table), pmd5, timestamp);
//	psc->node_list[idx]->node_addr.sin_addr.s_addr = inet_addr(addr);
//	psc->node_list[idx]->node_addr.sin_family = AF_INET;
//	psc->node_list[idx]->node_addr.sin_port = htons(33333);
//
//	return 0;
//}

//	struct sigaction sact;
//	int bytes = 0;
//	memset(&sact, 0, sizeof(struct sigaction));

	// init tunnel

//	struct svpn_clients* sc = &psc->svpn_clients;
//	sc->client_count = 0;
//	//list_init(&sc->client_list);
//	memset(sc->client_list, 0, sizeof(sc->client_list));
//	memset(sc->client_by_local_addr, 0, sizeof(sc->client_by_local_addr));
//
//	// init socket
//	if(svpn_sock_create(psc, port) < 0) {
//		free(psc);
//		return NULL;
//	}
//

//	psc->client_count = c_count;
//	bytes = sizeof(struct svpn_client_node*) * c_count;
//	psc->node_list = (struct svpn_client_node**) malloc(bytes);
//	memset(psc->node_list, 0, bytes);

	//signal(SIGUSR1, svpn_sig_handler);
//	sact.sa_handler = svpn_sig_handler;
//	sact.sa_flags &= ~SA_RESTART;
//	sigaction(SIGUSR1, &sact, &(psc->old_act));

//out:
