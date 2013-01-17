//
#include "client.h"
#include "server.h"


// create client struct in memory
// return the client_id, negative if error
int cache_client(int sock_id, const char* username);

// destroy client struct
// return negative it error
int uncache_client(int client_id);

// find the client by its name
// return the client_id, negative if error
int find_client_by_name(const char* username);

// find the client by its name
// return the clien id, negative if error
int find_client_by_remote_addr(int sock_id, struct sockaddr_storage* addr, socklen_t len);

int find_client_by_local_addr(int sock_id, struct sockaddr_storage* addr, socklen_t len);

// set the client status to ready, initialize data
// containing login and keepalive
// return negative if failed
int activate_client(int client_id);

// set client status to illegal, forbid all communication
int deactivate_client(int client_id);

// get the status of the client
// return client status(positive value)
int check_client_status(int client_id);

// find a unused ip address
// return the address, negative if error
uint32_t get_unused_local_address();

// initialize the client management
int svpn_client_init()
{
	psc->clients.client_count = 0;
	memset(psc->clients.clients_by_local_addr, 0,
			sizeof(psc->clients.clients_by_local_addr));
}

// release
int svpn_client_release()
{
	size_t i;
	for (i = 0; i < psc->clients.client_count; i++)
	{
hhhh
	}
}
