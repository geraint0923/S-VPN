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

#include "client.h"
#include "fd.h"
#include "server.h"
#include "../compress/compress.h"

using namespace std;

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

	// parameter optimization TODO:
	{
		int n = 1024 * 8;
		unsigned int slen = sizeof(n);
		if (setsockopt(ssock->socket_fd, SOL_SOCKET, SO_RCVBUF, &n, slen) == -1)
			printf("???\n");
		if (getsockopt(ssock->socket_fd, SOL_SOCKET, SO_RCVBUF, &n, &slen) == -1)
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

int svpn_socket_create_ex(struct svpn_socket* ssock,
		const char* addr, unsigned short port, int flags)
{
	// domain name unsupported currently
	int ret;
	struct sockaddr_storage saddr;
	// create address
	if (flags & SOCKET_IPV6)
	{
		ret = inet_pton(AF_INET6, addr, &saddr);
		((struct sockaddr_in6*)&saddr)->sin6_port = htons(port);
	}
	else if (flags & SOCKET_IPV4)
	{
		saddr.ss_family = AF_INET;
		ret = inet_pton(AF_INET, addr, &(((struct sockaddr_in*)&saddr)->sin_addr));
		((struct sockaddr_in*)&saddr)->sin_port = htons(port);
	}
	else
		ret = -1;
	if (ret < 0)
		return ret;

	return svpn_socket_create(ssock, &saddr, flags);
}

int svpn_socket_release(struct svpn_socket* ssock)
{
	return close(ssock->socket_fd);
}

int svpn_sockets_init()
{
	int ret;
	rapidxml::xml_node<>* xnetwork = psc->config->first_node("network");
	if (xnetwork == NULL)
		return -1;

	psc->sockets.socket_count = 0;
	// first traverse, count only
	rapidxml::xml_node<>* xsocket;
	for (xsocket = xnetwork->first_node("socket"); xsocket; xsocket = xsocket->next_sibling("socket"))
		psc->sockets.socket_count++;
	psc->sockets.sockets_list = new svpn_socket[psc->sockets.socket_count];
	// second traverse, initialize
	struct svpn_socket* ssock = &psc->sockets.sockets_list[0];
	for (xsocket = xnetwork->first_node("socket"); xsocket; xsocket = xsocket->next_sibling("socket"))
	{
		int sflag = 0;
		const char* xflag_type = xsocket->first_attribute("type")->value();
		const char* xflag_protocol = xsocket->first_attribute("protocol")->value();

		if (strcasecmp(xflag_type, "IPv4") == 0)
			sflag |= SOCKET_IPV4;
		else if (strcasecmp(xflag_type, "IPv6") == 0)
			sflag |= SOCKET_IPV6;
		else
		{
			delete [] psc->sockets.sockets_list;
			return -1;
		}

		if (strcasecmp(xflag_protocol, "TCP") == 0)
			sflag |= SOCKET_TCP;
		else if (strcasecmp(xflag_protocol, "UDP") == 0)
			sflag |= SOCKET_UDP;
		else
		{
			delete [] psc->sockets.sockets_list;
			return -1;
		}

		if ((ret < svpn_socket_create_ex(ssock, xsocket->first_attribute("address")->value(),
				atoi(xsocket->first_attribute("port")->value()), sflag)) < 0)
		{
			fprintf(stderr, "error creating socket %08x\n", ssock);
			continue;
		}
		ssock++;
	}
	if (ssock == psc->sockets.sockets_list) // no sockets created
	{
		delete [] psc->sockets.sockets_list;
		return -1;
	}
	// restrict sockets count, ignore those failed ones
	// maybe some waste in memory, don't mind the details . FIXME: reallocate memory
	psc->sockets.socket_count = ssock - psc->sockets.sockets_list;
	return 0;
}

int svpn_sockets_release()
{
	return 0;
}

int socket_recvfrom(int sock_id, unsigned char* buffer, size_t* len, int* flag,
		struct sockaddr_storage* addr, socklen_t* addr_len)
{
	fd_t fd = psc->sockets.sockets_list[sock_id].socket_fd;
	// only for udp protocol, ipv4 & v6 compatible
	unsigned char __rawbuf[BUFFER_LENGTH];
	unsigned char* rawbuf = __rawbuf;
	unsigned char sflag;
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
	// get flag
	sflag = rawbuf[0];
	*flag = rawbuf[0];
	recved--;
	rawbuf++;
	// decrypt
	if (!(sflag & PACKAGE_UNENCRYPTED))
	{
		// check user status
		int client_id = find_client_by_remote_addr(sock_id, addr, *addr_len);
		if (client_id < 0)
		{
			fprintf(stderr, "[Error] Could not find correspond user.\n");
			return -SVPN_USER_NOTFOUND;
		}
		svpn_decrypt(&psc->clients.clients_list[client_id]->table,
				rawbuf, rawbuf, recved);
	}
	// uncompress
	if (sflag & PACKAGE_COMPRESSED)
		decompress(buffer, (size_t*)&recved, rawbuf, recved);
	else
		memcpy(buffer, rawbuf, recved);
	*len = recved;
	return 0;
}

int socket_sendto(int sock_id, const unsigned char* buffer, size_t len, int flag,
		const struct sockaddr_storage* addr, socklen_t addr_len, int client_id)
{
	fd_t fd = psc->sockets.sockets_list[sock_id].socket_fd;
	// only for udp protocol, ipv4 & v6 compatible
	unsigned char __rawbuf[BUFFER_LENGTH];
	unsigned char* rawbuf = __rawbuf +1;
	// compress
	if (flag & PACKAGE_COMPRESSED)
		compress(rawbuf, &len, buffer, len);
	else
		memcpy(rawbuf, buffer, len);
	// encrypt
	if (!(flag & PACKAGE_UNENCRYPTED))
	{
		// check user status
		if (client_id < 0)
			printf("error.\n");
		svpn_decrypt(&psc->clients.clients_list[client_id]->table,
				rawbuf, rawbuf, len);
	}
	rawbuf--;
	len++;
	*rawbuf = flag;
	return sendto(fd, rawbuf, len, 0, (struct sockaddr*)addr, addr_len);
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
