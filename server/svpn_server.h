#ifndef __SVPN_SERVER_H__
#define __SVPN_SERVER_H__

#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include "crypt.h"

#define DEV_NAME_LEN	128

struct svpn_client_node {
	struct sockaddr_in node_addr;
	struct CodeTable table;
};

struct svpn_server {
	char dev_name[DEV_NAME_LEN];
	int sock_fd;
	struct sockaddr_in server_addr; 
	int tun_fd;
	pthread_t recv_tid;
	pthread_t send_tid;
	int recv_thread_on;
	int send_thread_on;
	struct svpn_client_node **node_list;
	struct sigaction old_act;
	int client_count;
};

/*
   client_count should be less than 250
*/
struct svpn_server *svpn_server_init(unsigned short port, int client_count);

/* start the socket recv thread with UDP 
   will write the tunnel fd
 */
int svpn_server_start_recv_thread(struct svpn_server *psc);

/* start the socket send thread with UDP 
   will read the tunnel fd
 */
int svpn_server_start_send_thread(struct svpn_server *psc);

int svpn_server_stop_recv_thread(struct svpn_server *psc);

int svpn__server_stop_send_thread(struct svpn_server *psc);

int svpn_server_wait_recv_thread(struct svpn_server *psc);

int svpn_server_wait_send_thread(struct svpn_server *psc);

int svpn_server_release(struct svpn_server *psc);

int svpn_server_available_index(struct svpn_server *psc);

int svpn_server_remove_client(struct svpn_server *psc, int idx);

int svpn_server_add_client(struct svpn_server *psc, int idx, char *addr,
		unsigned char *pwd_md5, long long timestamp);

#endif
