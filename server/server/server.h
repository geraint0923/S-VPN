#ifndef __SVPN_SERVER_H__
#define __SVPN_SERVER_H__

#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include "crypt.h"
#include "client.h"
#include "fd.h"
#include "../xml/rapidxml.hpp"

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
	const rapidxml::xml_node<>* config;
	//
	int running;
};

extern struct svpn_server* psc;

/*
   client_count should be less than 250
*/
int svpn_server_init(const rapidxml::xml_node<>* config);

int svpn_server_main();

int svpn_server_release();

int client_sendto(int client_id, const unsigned char* buffer, size_t len,
				int flag);

int control_package_handler(int sock_id, const unsigned char* buffer, int len, 
				int flag, const struct sockaddr_storage* src_addr,
				socklen_t src_len);
				
int package_forwarding(int sock_id, const unsigned char* buffer, int len,
				int flag, const struct sockaddr_storage* src_addr,
				socklen_t src_len);

#endif
