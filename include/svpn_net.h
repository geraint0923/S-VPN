#ifndef __SVPN_NET_H__
#define __SVPN_NET_H__

struct svpn_net_ipv4_header {
//	unsigned char ihl:4;
	unsigned char version;
	unsigned char tos;
	unsigned short tot_len;
	unsigned short id;
	unsigned short frag_off;
	unsigned char ttl;
	unsigned char protocol;
	unsigned short chk_sum;
	unsigned char src_ip[4];
	unsigned char dst_ip[4];
};

#endif
