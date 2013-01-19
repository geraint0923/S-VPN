//
#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#if defined(__unix)
#include <sys/socket.h>
#include <netinet/in.h>
#elif defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WinSock2.h>
#endif

#if defined(__unix)
typedef int socket_t;
typedef int fd_t;
#elif defined(WIN32)
typedef SOCKET socket_t;
typedef HANDLE fd_t;
#endif

extern long long now;

#define BUFFER_LENGTH	4096
#define BUFFER_LEN	BUFFER_LENGTH
#define BUFFER_SIZE	BUFFER_LENGTH

#define PACKAGE_CONTROL 0x1
#define PACKAGE_UNENCRYPTED 0x2
#define PACKAGE_ENCRYPTED 0x0
#define PACKAGE_COMPRESSED 0x4

#define PACKAGE_CONTROL_USERNAME 1
#define PACKAGE_CONTROL_TIMESTAMP 2
#define PACKAGE_CONTROL_KEEPALIVE 3
#define PACKAGE_CONTROL_LOGIN 4

#define SVPN_MAGIC_NUMBER 0x38f38296

#define SVPN_OK 0
#define SVPN_USER_NOTFOUND 101
#define SVPN_USER_INVALID 102
#define SVPN_WRONG_PASSWORD 103

#define SVPN_PASSED 0
#define ERROR_INVALID_PACKAGE 203

inline uint64_t ntoh64(const uint64_t i)
{
#ifdef htobe64
	return htobe64(i);
#else // only Win8 has ntohll
	uint64_t res;
	uint32_t* dst = (uint32_t*)&i;
	uint32_t* src = (uint32_t*)&res;
	dst[0] = ntohl(src[1]);
	src[1] = ntohl(src[0]);
	return res;
#endif
}

struct control_package_login
{
	uint8_t status;
	// login information
	uint32_t local_address;
	uint32_t network_mask;
	uint32_t remote_address;
	//
	uint64_t valid_time;
	//
	char _end[0];
};

inline void ntoh_control_package_login(struct control_package_login* c)
{
	c->status = ntohl(c->status);
	c->valid_time = ntoh64(c->valid_time);
	c->network_mask = ntohl(c->network_mask);
	c->local_address = ntohl(c->local_address);
	c->remote_address = ntohl(c->remote_address);
};

struct control_package_keepalive
{
	uint32_t magic_number;
	uint32_t options;
	//
	char _end[0];
};

inline void ntoh_control_package_keepalive(struct control_package_keepalive* c)
{
	c->magic_number = ntohl(c->magic_number);
	c->options = ntohl(c->options);
};

struct control_package_timestamp
{
	uint64_t timestamp;
	//
	char _end[0];
};

inline void ntoh_control_package_timestamp(struct control_package_timestamp* c)
{
	c->timestamp = ntoh64(c->timestamp);
}

struct control_package_username
{
	char username[1]; // one byte wasted for MSVC
	//
	char _end[0];
};

#define PACKAGE_SIZE(m) offsetof(control_package, m)

#define PACKAGE_SIZE_CONTROL_USERNAME PACKAGE_SIZE(content.pkg_username._end)
#define PACKAGE_SIZE_CONTROL_TIMESTAMP PACKAGE_SIZE(content.pkg_timestamp._end)
#define PACKAGE_SIZE_CONTROL_KEEPALIVE PACKAGE_SIZE(content.pkg_keepalive._end)
#define PACKAGE_SIZE_CONTROL_LOGIN PACKAGE_SIZE(content.pkg_login._end)

// socket collection
// remember pack(1): TODO:
struct control_package
{
	uint8_t type;
	union
	{
		struct control_package_username pkg_username;
		struct control_package_timestamp pkg_timestamp;
		struct control_package_keepalive pkg_keepalive;
		struct control_package_login pkg_login;
	}
	content;
};


#ifdef __cplusplus
}
#endif

#endif
