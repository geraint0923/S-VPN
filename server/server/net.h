#ifndef __SVPN_NET_H__
#define __SVPN_NET_H__

struct net_ipv4_header
{
	unsigned char version;
	unsigned char tos;
	unsigned short tot_len;
	unsigned short id;
	unsigned short frag_off;
	unsigned char ttl;
	unsigned char protocol;
	unsigned short chk_sum;
	uint32_t src_ip;
	uint32_t dst_ip;
};

#endif
