#ifndef __SVPN_SERVER_H__
#define __SVPN_SERVER_H__

#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include "crypt.h"

typedef int socket_t;
typedef int fd_t;

struct svpn_server
{
	// tun
	uint32_t remote_addr;
	uint32_t subnet_mask;
	// sockets & tun
	struct svpn_tun tun;
	struct svpn_sockets sockets;
	struct svpn_clients clients;
	//
	mxml_node_t config_file;
	//
	int running;
};

/*
   client_count should be less than 250
*/
struct svpn_server *svpn_server_init(const xml_node_t* config);

int svpn_server_release();

int svpn_server_main();

int client_sendto(int client_id, const unsigned char* buffer, size_t len,
				int flag);

int control_package_handler(int sock_id, const unsigned char* buffer, int len, 
				int flag, const struct sockaddr_storage* src_addr,
				socklen_t src_len);
				
int package_forwarding(int sock_id, const unsigned char* buffer, int len,
				int flag, const struct sockaddr_storage* src_addr,
				socklen_t src_len);

int handle

#endif
