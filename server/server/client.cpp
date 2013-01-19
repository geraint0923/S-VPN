//

#include <string.h>
#include <stdio.h>
#include "../md5/md5.h"
#include "../crypt/crypt.h"
#include "../xml/rapidxml.hpp"
#include "client.h"
#include "server.h"

// find the client by its name
// return the client_id, negative if error
static struct svpn_client* find_client_by_name(const char* username)
{
	int ret;
	//
	const rapidxml::xml_node<>* xusers = psc->config->first_node("users");
	const rapidxml::xml_node<>* xuser;
	for (xuser = xusers->first_node(); xuser; xuser = xuser->next_sibling())
	{
		const char* xname = xuser->first_attribute("name")->value();
		if (strcmp(xname, username) == 0)
		{
			goto found;
			break;
		}
	}
	return NULL;

found:
	struct svpn_client* cli = (struct svpn_client*)malloc(sizeof(struct svpn_client));
	cli->status = CLIENT_STATUS_CACHED;
	unsigned char pass_md5[16];
	if (parse_md5(pass_md5, xuser->first_attribute("password")->value()) < 0)
	{
		free(cli);
		return NULL;
	}

	build_table(&cli->table, pass_md5, now);
	return cli;
}

// create client struct in memory
// return the client_id, negative if error
int cache_client(int sock_id, const char* username,
		const struct sockaddr_storage* addr, socklen_t addr_len)
{
	int client_id;
	struct svpn_client* nclient = find_client_by_name(username);
	if (nclient == NULL)
		return -SVPN_USER_NOTFOUND;
	// get the client_id
	client_id = psc->clients.client_count;
	psc->clients.client_count++;
	psc->clients.clients_list[client_id] = nclient;
	// initialize other fields
	nclient->id = client_id;
	nclient->last_login = now;
	memcpy(&nclient->node_addr, &addr, addr_len);
	nclient->node_addr_len = addr_len;
	nclient->socket_id = sock_id;
	return client_id;
}

// destroy client struct
// return negative it error
int uncache_client(int client_id)
{
	printf("Not implemented.\n");
	return -1;
}

// find the client by its name
// return the client id, negative if error
int find_client_by_remote_addr(int sock_id,
		const struct sockaddr_storage* addr, socklen_t len)
{
	int i;
	for (i = 0; i < psc->clients.client_count; i++)
	{
		const struct svpn_client* cli = psc->clients.clients_list[i];
		if (cli == NULL)
			continue;
		if (cli->socket_id == sock_id && memcmp(&cli->node_addr, addr, len) == 0)
			return i;
	}
	return -SVPN_USER_NOTFOUND;
}

int find_client_by_local_addr(uint32_t addr)
{
	// TODO: currently only support C class IP address
	int local_id = ntohl(addr & (~psc->subnet_mask));
	if (psc->clients.clients_by_local_addr[local_id] != NULL)
		return psc->clients.clients_by_local_addr[local_id]->id;
	return -SVPN_USER_NOTFOUND;
}

// set the client status to ready, initialize data
// containing login and keepalive
// return negative if failed
int activate_client(int client_id, uint32_t localaddr)
{
	int local_id = ntohl(localaddr & (~psc->subnet_mask));
	psc->clients.clients_list[client_id]->status = CLIENT_STATUS_CONNECTED;
	if (psc->clients.clients_by_local_addr[local_id] != NULL)
		return -1;
	psc->clients.clients_by_local_addr[local_id] = psc->clients.clients_list[client_id];
	return 0;
}

// set client status to illegal, forbid all communication
int deactivate_client(int client_id)
{
	psc->clients.clients_list[client_id]->status = CLIENT_STATUS_EXPIRED;
	return 0;
}

// get the status of the client
// return client status(positive value)
int check_client_status(int client_id)
{
	return (client_id < psc->clients.client_count &&
			psc->clients.clients_list[client_id]->status == CLIENT_STATUS_CONNECTED);
}

// find a unused ip address
// return the address, negative if error
uint32_t get_unused_local_address()
{
	uint32_t i;
	// FIXME: only C class address is supported here
	for (i = 0; i < 256; i++)
		if (psc->clients.clients_by_local_addr[i] == NULL)
			return (psc->remote_addr & psc->subnet_mask) | htonl(i);
	return 0;
}

// initialize the client management
int svpn_clients_init()
{
	//
	psc->clients.client_count = 0;
	memset(psc->clients.clients_by_local_addr, 0,
			sizeof(psc->clients.clients_by_local_addr));
	return 0;
}

// release
int svpn_clients_release()
{
//	printf("svpn_client_release not implemented.\n"); // TODO:FIXME:NOT!
//	size_t i;
//	for (i = 0; i < psc->clients.client_count; i++)
	return 0;
}
