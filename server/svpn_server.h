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
	struct sockaddr_in remote_addr;
	uint32_t subnet_mask;
	int tun_fd;
	// sockets
	size_t sockets_count;
	struct svpn_socket* socket_list;
	// clients
	struct svpn_clients;
	// users
	mxml_node_t config_file;
	//
	int running;
};

#define DEV_NAME_LEN	128

struct svpn_socket
{
	struct sockaddr_storage server_addr;
	int mode;
	socket_t socket_fd;
	// status
	size_t packages_received;
	size_t packages_sent;
	size_t bytes_received;
	size_t bytes_sent;
	// status - speed
	struct timeval unused;
};

struct svpn_clients
{
	size_t client_count;
	struct svpn_client* client_list[256];
	rb_tree* client_by_node_addr;
	struct svpn_client* client_by_local_addr[256]; // 192.168.3.2 ~ 192.168.3.254
};

//////////
//////////
//////////
struct svpn_client
{
	struct CodeTable* table;
	struct sockaddr_storage node_addr;
	struct sockaddr_storage local_addr;
	int status;
	int option;
	struct timeval last_access;
	int socket_id;
}

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
	pthread_t handle_tid;
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

int svpn_server_start_handle_thread(struct svpn_server *psc);

int svpn_server_stop_recv_thread(struct svpn_server *psc);

int svpn__server_stop_send_thread(struct svpn_server *psc);

int svpn_server_wait_recv_thread(struct svpn_server *psc);

int svpn_server_wait_send_thread(struct svpn_server *psc);

int svpn_server_wait_handle_thread(struct svpn_server *psc);

int svpn_server_release(struct svpn_server *psc);

int svpn_server_available_index(struct svpn_server *psc);

int svpn_server_remove_client(struct svpn_server *psc, int idx);

int svpn_server_add_client(struct svpn_server *psc, int idx, char *addr,
		unsigned char *pwd_md5, long long timestamp);

#endif
