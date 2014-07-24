#ifndef __SVPN_FD_H__
#define __SVPN_FD_H__

#include "svpn_server.h"

int svpn_tun_create(const char *dev_name);

int svpn_sock_create(struct svpn_server *psc, unsigned short port);

#endif 
