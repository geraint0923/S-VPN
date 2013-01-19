#ifndef __SVPN_FD_H__
#define __SVPN_FD_H__

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
	struct svpn_socket* sockets_list;
};

int svpn_tun_create(const char *dev_name);

int svpn_tun_release();

int svpn_socket_create(struct svpn_socket* ssock,
		const struct sockaddr_storage* addr, int flags);
int svpn_socket_create_ex(struct svpn_socket* ssock,
		const char* addr, unsigned short port, int flags);

int svpn_socket_release();

int svpn_sockets_init();
int svpn_sockets_release();

int socket_recvfrom(int sock_id, unsigned char* buffer, size_t* len, int* flag,
		struct sockaddr_storage* addr, socklen_t* addr_len);

int socket_sendto(int sock_id, const unsigned char* buffer, size_t len, int flag,
		const struct sockaddr_storage* addr, socklen_t addr_len, int client_id);

int tun_recv(unsigned char* buffer, size_t* len, int* flag);
int tun_send(const unsigned char* buffer, size_t len, int flag);

#endif 
