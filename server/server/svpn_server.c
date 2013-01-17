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

typedef unsigned char byte;

#define PACKAGE_CONTROL 0x1
#define PACKAGE_UNENCRYPTED 0x2
#define PACKAGE_COMPRESSED 0x4

#define PACKAGE_CONTROL_USERNAME 1
#define PACKAGE_CONTROL_TIMESTAMP 2
#define PACKAGE_CONTROL_KEEPALIVE 3
#define PACKAGE_CONTROL_LOGIN 4

#define SVPN_MAGIC_NUMBER 0x38f38296

#define SOCKET_TCP 0x1
#define SOCKET_UDP 0x2

#define SOCKET_IPV4 0x4
#define SOCKET_IPV6 0x8

#define SVPN_OK 0
#define SVPN_USER_NOTFOUND 101
#define SVPN_USER_INVALID 102
#define SVPN_WRONG_PASSWORD 103

#define SVPN_PASSED 0
#define ERROR_INVALID_PACKAGE 203

uint64_t ntohq(uint64_t i)
{
	uint64_t res;
	uint32_t* dst = (uint32_t*)&i;
	uint32_t* src = (uint32_t*)&res;
	dst[0] = ntohl(src[1]);
	src[1] = ntohl(src[0]);
	return res;
}

// socket collection
// remember pack(1): TODO:
struct control_package
{
	uint8_t mark;
	uint8_t type;
	union
	{
		struct control_package_username pkg_username;
		struct control_package_timestamp pkg_timestamp;
		struct control_package_keepalive pkg_keepalive;
		struct control_package_login pkg_login;
	}
	content;
};

struct control_package_login
{
	uint8_t status;
	// login information
	struct sockaddr_in local_addresss;
	uint32_t network_mask;
	struct sockaddr_in remote_address;
	//
	uint64_t valid_time;
	//
	char _pad[0];
};

void ntoh_control_package_login(struct control_package_login* c)
{
	c->status = ntohl(c->status);
	c->valid_time = ntohq(c->valid_time);
	c->network_mask = ntohl(c->network_mask);
};

struct control_package_keepalive
{
	uint32_t magic_number;
	uint32_t options;
	//
	char _pad[0];
};

void ntoh_control_package_keepalive(struct control_package_keepalive* c)
{
	c->magic_number = ntohl(c->magic_number);
	c->options = ntohl(c->options);
};

struct control_package_timestamp
{
	uint64_t timestamp;
	//
	char _pad[0];
};

void ntoh_control_package_timestamp(struct control_package_timestamp* c)
{
	c->timestamp = ntohq(c->timestamp);
}

struct control_package_username
{
	char username[0];
	//
	char _pad[0];
};

// minus 1 for mark
#define PACKAGE_SIZE(m) (offsetof(control_package, m) - 1)

#define PACKAGE_SIZE_CONTROL_USERNAME PACKAGE_SIZE(content.pkg_username._pad)
#define PACKAGE_SIZE_CONTROL_TIMESTAMP PACKAGE_SIZE(content.pkg_timestamp._pad)
#define PACKAGE_SIZE_CONTROL_KEEPALIVE PACKAGE_SIZE(content.pkg_keepalive._pad)
#define PACKAGE_SIZE_CONTROL_LOGIN PACKAGE_SIZE(content.pkg_login._pad)

int control_package_handler(int sock_id, const unsigned char* buffer,
							const struct sockaddr_storage* src_addr,
							socklen_t src_len)
{
	int ret;
	const struct control_package* ctl_pkg = (const struct control_package*)buffer;
	// check control package
	if (!(ctl_pkg->mark & PACKAGE_CONTROL))
		return -ERROR_INVALID_PACKAGE;
	switch (ctl_pkg->type)
	{
	case PACKAGE_CONTROL_USERNAME:
		{
			// add to candidate, ret = uid
			ret = add_client(sock_id, ctl_pkg.content.pkg_username.username,
						src_addr, src_len);
			// send replay
			struct control_package reply;
			reply.type = PACKAGE_CONTROL_TIMESTAMP;
			if (ret < 0)
			{
				reply.content.pkg_timestamp.timestamp = 0LL;
				svpn_socket_sendto(sock_id, &reply.type,
					PACKAGE_UNENCRYPTED | PACKAGE_CONTROL,
					PACKAGE_SIZE_CONTROL_TIMESTAMP, src_addr, src_len);
				return ret;
			}
			else
			{
				reply.content.pkg_timestamp.timestamp = now;
				svpn_client_sendto(ret, &reply.type,
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
			if (ctl_pkg.content.pkg_keepalive.magic_number !=
				SVPN_MAGIC_NUMBER)
			{
				reply.content.pkg_login.status = SVPN_WRONG_PASSWORD;
				svpn_client_sendto(ret, &reply.type,
					PACKAGE_ENCRYPTED | PACKAGE_CONTROL,
					PACKAGE_SIZE_CONTROL_LOGIN);
				return 0;
			}
			// TODO: check options. e.g. ipv6
			// add to candidate, ret = uid
			ret = find_client(sock_id, src_addr, src_len);
			if (ret < 0)
				return 0;
			uid = ret;
			
			ret = activate_client(ret, SVPN_OK);
			if (ret < 0)
				return 0;
				
			// send replay
			reply.content.pkg_login.status = SVPN_OK;
			reply.content.pkg_login.remote_address = psc->remote_addr;
			reply.content.pkg_login.network_mask = psc->subnet_mask;
			reply.content.pkg_login.local_address = svpn_client_getunused_localaddr();
			reply.content.pkg_login.valid_time = psc->client_valid_time;
			svpn_client_sendto(uid, &reply.type,
				PACKAGE_ENCRYPTED | PACKAGE_CONTROL,
				PACKAGE_SIZE_CONTROL_TIMESTAMP);
			return 0;
		}
		break;
	default:
		return -ERROR_INVALID_PACKAGE;
		break;
	}
	return ret;
}


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
				
			if (*buffer & PACKAGE_CONTROL)
				ret = handle_control_package();
			if (ret == 0)
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
