//
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "common.h"
#include "../crypt/crypt.h"

#define CLIENT_STATUS_NOTEXIST 101
#define CLIENT_STATUS_CACHED 102
#define CLIENT_STATUS_CONNECTED 200
#define CLIENT_STATUS_EXPIRED 103
#define CLIENT_STATUS_ABNORMAL 104

struct svpn_client
{
	int id;
	struct code_table table;
	struct sockaddr_storage node_addr;
	socklen_t node_addr_len;
	uint32_t local_addr;
	int status;
	int option;
	long long last_login;
	int socket_id;
};

struct svpn_clients
{
	size_t client_count;
	struct svpn_client* clients_list[256];
	struct svpn_client* clients_by_local_addr[256]; // 192.168.3.2 ~ 192.168.3.254
	long long valid_time;
};

// create client struct in memory
// return the client_id, negative if error
int cache_client(int sock_id, const char* username,
		const struct sockaddr_storage* addr, socklen_t addr_len);

// destroy client struct
// return negative it error
int uncache_client(int client_id);

// find the client by its name
// return the client_id, negative if error
//static int find_client_by_name(const char* username);

// find the client by its name
// return the client id, negative if error
int find_client_by_remote_addr(int sock_id,
		const struct sockaddr_storage* addr, socklen_t len);

int find_client_by_local_addr(uint32_t addr);

// set the client status to ready, initialize data
// containing login and keepalive
// return negative if failed
int activate_client(int client_id, uint32_t localaddr);

// set client status to illegal, forbid all communication
int deactivate_client(int client_id);

// get the status of the client
// return client status(positive value)
int check_client_status(int client_id);

// find a unused ip address
// return the address, negative if error
uint32_t get_unused_local_address();

// initialize the client management
int svpn_clients_init();

// release
int svpn_clients_release();

#endif
