#include "svpn_fd.h"
#include "server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "svpn_client.h"
#include "svpn_fd.h"
#include "svpn_server.h"

#define BUFFER_LENGTH 2000

#define	TUN_DEFAULT_NAME "/dev/net/tun"

int svpn_tun_create(const char *dev_name)
{
	fd_t fd;
	int err;
	struct ifreq ifr;

	if (dev_name == NULL)
		dev_name = TUN_DEFAULT_NAME;

	if((fd = open(dev_name, O_RDWR)) < 0)
	{
		perror("tun_create");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if ((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0)
	{
		close(fd);
		return err;
	}
	return fd;
}

int svpn_tun_release()
{
	return close(psc->tun.tun_fd);
}

int svpn_socket_create(struct svpn_socket* ssock,
		const struct sockaddr_storage* addr, int flags)
{
	int addr_len;
	int protocol;

	if (addr->ss_family == AF_INET)
		addr_len = sizeof(struct sockaddr_in);
	else if (addr->ss_family == AF_INET6)
		addr_len = sizeof(struct sockaddr_in6);
	else
		return -1;

	if (flags & SOCKET_TCP)
		protocol = SOCK_STREAM;
	else if (flags & SOCKET_UDP)
		protocol = SOCK_DGRAM;
	else
		return -1;

	memcpy(&ssock->server_addr, ssock, addr_len);
	ssock->socket_fd = socket(addr->ss_family, protocol, 0);

	// parameter optimization
	{
		int n = 1024 * 8;
		int slen = sizeof(n);
		if (setsockopt(ssock->socket_fd, SOL_SOCKET, SO_RCVBUF, &n, slen) == -1) {
			printf("???\n");
		if (getsockopt(ssock->socket_fd, SOL_SOCKET, SO_RCVBUF, &n, &slen) == -1) {
			printf("wrong arg?\n");
			printf("recvbuf = %d\n", n);
	}

	if (ssock->socket_fd < 0)
		return -1;

	if (bind(ssock->socket_fd, (struct sockaddr*)addr, addr_len) < 0)
	{
		close(ssock->socket_fd);
		return -1;
	}

	ssock->mode = flags;
	ssock->bytes_received = 0;
	ssock->bytes_sent = 0;
	ssock->packages_received = 0;
	ssock->packages_sent = 0;

	return 0;
}

int svpn_socket_create(struct svpn_socket* ssock,
		const char* addr, int flags)
{
	// domain name unsupported currently
	int ret;
	struct sockaddr_storage saddr;
	// create address
	if (flags & SOCKET_IPV6)
		ret = inet_pton(AF_INET6, addr, saddr);
	else if (flags & SOCKET_IPV4)
		ret = inet_pton(AF_INET, addr, saddr);
	else
		ret = -1;
	if (ret < 0)
		return ret;

	return svpn_socket_create(ssock, saddr, flags);
}

int svpn_socket_release(struct svpn_socket* ssock)
{
	return close(ssock->socket_fd);
}

int svpn_sockets_init()
{
	psc->sockets.socket_count = 0;
	// load from config
	xml_node_t rt = ;
}

int svpn_sockets_release()
{
}

int socket_recvfrom(int sock_id, unsigned char* buffer, int* len, int* flag,
		struct sockaddr_storage* addr, socklen_t* addr_len)
{
	fd_t fd = psc->sockets.sockets_list[sock_id].socket_fd;
	// only for udp protocol, ipv4 & v6 compatible
	unsigned char* rawbuf[BUFFER_LENGTH];
	int recved;
	if ((recved = recvfrom(fd, rawbuf, BUFFER_LENGTH, 0,
			(struct sockaddr*)addr, addr_len)) < 0)
		return recved;
	//
	if (recved == 0)
	{
		*len = 0;
		return 0;
	}
	//
	recved--;
	*flag = rawbuf[0];
	// uncompress
	if (rawbuf[0] & PACKAGE_COMPRESSED)
	{
		decompress(buffer, &recved, rawbuf +1, recved);
	}
	else
		memcpy(buffer, rawbuf + 1, recved);
	// decrypt
	if (!(rawbuf[0] & PACKAGE_UNENCRYPTED))
	{
		// check user status
		int client_id = find_client_by_remote_addr(sock_id, addr, addr_len);
		if (client_id < 0)
		{
			fprintf(stderr, "[Error] Could not find correspond user.\n");
			return -SVPN_USER_NOTFOUND;
		}
		decrypt(psc->clients.clients_list[client_id]->table,
				buffer, recved, buffer, &recved);
	}
	*len = recved;
	return 0;
}

int socket_sendto(int sock_id, const unsigned char* buffer, size_t len, int flag,
		const struct sockaddr_storage* addr, socklen_t addr_len)
{
	fd_t fd = psc->sockets.sockets_list[sock_id].socket_fd;
	// only for udp protocol, ipv4 & v6 compatible
	unsigned char* rawbuf[BUFFER_LENGTH];
	// encrypt
	if (!(flag & PACKAGE_UNENCRYPTED))
	{
		// check user status
		// don't needed anymore, the address must be fetched from the cache
		decrypt(psc->clients.clients_list[client_id]->table,
				buffer, len, buffer, &len);
	}
	// compress
	if (rawbuf[0] & PACKAGE_COMPRESSED)
	{
		compress(rawbuf +1, buffer, &len, buffer, len);
	}
	else
		memcpy(rawbuf + 1, buffer, len);

	rawbuf[0] = flag;
	len++;
	return sendto(fd, rawbuf, len, (struct sockaddr*)addr, addr_len);
}

int tun_recv(unsigned char* buffer, size_t* len, int* flag)
{
	*len = read(psc->tun.tun_fd, buffer, BUFFER_LENGTH);
	// TODO: statistics
	return *len;
}

int tun_send(const unsigned char* buffer, size_t len, int flag)
{
	size_t ret = write(psc->tun.tun_fd, buffer, len);
	// TODO: statistics
	return ret;
}
