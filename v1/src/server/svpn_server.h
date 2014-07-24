#ifndef __SVPN_SERVER_H__
#define __SVPN_SERVER_H__

#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include "crypt.h"

#define DEV_NAME_LEN	128

struct svpn_client_node
{
	struct sockaddr_in6 addr;
	struct CodeTable table;
};

struct svpn_server
{
	char dev_name[DEV_NAME_LEN];
	unsigned char local_addr[4];
	int sock_fd;
	struct sockaddr_in6 server_addr;
	int tun_fd;
	struct svpn_client_node* clients[256];
	struct sigaction old_act;
};

/*
   client_count should be less than 250
*/
struct svpn_server *svpn_server_init(const char* configfile);

int svpn_server_init_client(struct svpn_server *psc, const char* userlist);

int svpn_server_handle_thread(struct svpn_server* pvoid);

#endif
