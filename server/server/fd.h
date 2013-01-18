#ifndef __SVPN_FD_H__
#define __SVPN_FD_H__

#include "server.h"

#define SOCKET_TCP 0x1
#define SOCKET_UDP 0x2

#define SOCKET_IPV4 0x4
#define SOCKET_IPV6 0x8

struct svpn_tun
{
	fd_t tun_fd;
	// statistics
	size_t packages_received;
	size_t packages_sent;
	size_t bytes_received;
	size_t bytes_sent;
};

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
	long long unused;
};

struct svpn_sockets
{
	int socket_count;
	struct svpn_socket sockets_list;
};

int svpn_tun_create(const char *dev_name);

struct svpn_socket* svpn_socket_create(const struct sockaddr_storage* addr);
struct svpn_socket* svpn_socket_create(int flag, const char* addr);

int svpn_tun_release();

int svpn_socket_release();

int socket_recvfrom(int sock_id, unsigned char* buffer, int* len, int* flag,
		struct sockaddr_storage* addr, socklen_t* addr_len);

int socket_sendto(int sock_id, const unsigned char* buffer, int len, int flag,
		const struct sockaddr_storage* addr, socklen_t addr_len);

int tun_recv(unsigned char* buffer, int* flag);
int tun_send(const unsigned char* buffer, int flag);

#endif 
