#ifndef __SVPN_CLIENT_H__
#define __SVPN_CLIENT_H__

#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include "crypt.h"

#define DEV_NAME_LEN	128

struct svpn_client {
	char dev_name[DEV_NAME_LEN];
	int sock_fd;
	struct sockaddr_in server_addr; 
	int tun_fd;
	pthread_t recv_tid;
	pthread_t send_tid;
	pthread_t handle_tid;
	int recv_thread_on;
	int send_thread_on;
	struct CodeTable table;
	struct sigaction old_act;
};

struct svpn_client *svpn_init(char *addr, unsigned short port, 
		unsigned char *pwd_md5_16, long long timestamp);

/* start the socket recv thread with UDP 
   will write the tunnel fd
 */
int svpn_start_recv_thread(struct svpn_client *psc);

/* start the socket send thread with UDP 
   will read the tunnel fd
 */
int svpn_start_send_thread(struct svpn_client *psc);

int svpn_start_handle_thread(struct svpn_client *psc);

int svpn_stop_recv_thread(struct svpn_client *psc);

int svpn_stop_send_thread(struct svpn_client *psc);

int svpn_wait_recv_thread(struct svpn_client *psc);

int svpn_wait_send_thread(struct svpn_client *psc);

int svpn_wait_handle_thread(struct svpn_client *psc);

int svpn_release(struct svpn_client *psc);

#endif
