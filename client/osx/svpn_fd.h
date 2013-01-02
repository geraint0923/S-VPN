#ifndef __SVPN_FD_H__
#define __SVPN_FD_H__

#include "svpn_client.h"

int svpn_tun_create(char *dev_name);

int svpn_sock_create(struct svpn_client *psc, char *addr, unsigned short port);

#endif 
