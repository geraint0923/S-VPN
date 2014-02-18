#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "svpn_server.h"
#include "crypt.h"
#include "svpn_fd.h"
#include "svpn_net.h"
#include "util.h"

#define BUFFER_LEN	4096

static void svpn_sig_handler(int sig) {
	char buffer[] = "Signal?\n";
	int tmp = write(1, buffer, strlen(buffer));
}

struct tstat
{
	long long total_send;
	long long total_recv;
	long long last_send, last_recv;
	int total_pkgsend, total_pkgrecv;
	int last_pkgsend, last_pkgrecv;
	int speed_send, speed_recv;
	long long ts_last;
	long long ts_current;
};

long long tl_last = 0;
FILE* frec = NULL;


struct tstat stat;

#define OPT_TH 1024 * 1024
#define TIM_TH 1000000 * 2

char fmtstr[128];

inline void fillnum(char* s, int len, int num)
{
	int lenb = len;
	s += lenb -1;
	while (lenb > 0)
	{
		if (num == 0)
			*s = ' ';
		else if ((len - lenb) % 4 == 3)
			*s = ',';
		else
		{
			*s = '0' + num % 10;
			num /= 10;
		}

		s--;
		lenb--;
	}
}

inline void fillnum64(char* s, int len, long long num)
{
        int lenb = len;
        s += lenb -1;
        while (lenb > 0)
        {
                if (num == 0)
                        *s = ' ';
                else if ((len - lenb) % 4 == 3)
                        *s = ',';
                else
                {
                        *s = '0' + num % 10;
                        num /= 10;
                }

                s--;
                lenb--;
        }
}


inline void output_info()
{
	int tval;

	if (stat.total_send - stat.last_send < OPT_TH &&
		stat.total_recv - stat.last_recv < OPT_TH &&
		stat.ts_current - stat.ts_last < TIM_TH)
			return;

	tval = stat.ts_current - stat.ts_last;
	stat.speed_send = (stat.total_send - stat.last_send) * 1000000 / tval;
	stat.speed_recv = (stat.total_recv - stat.last_recv) * 1000000 / tval;

	fillnum64(fmtstr + 5, 14, stat.total_send);
	fillnum(fmtstr + 22, 9, stat.speed_send);
	fillnum64(fmtstr + 42, 14, stat.total_recv);
	fillnum(fmtstr + 59, 9, stat.speed_recv);
	fprintf(stderr, "%s", fmtstr);

	stat.ts_last = stat.ts_current;
	stat.last_send = stat.total_send;
	stat.last_recv = stat.total_recv;
	stat.last_pkgsend = stat.total_pkgsend;
	stat.last_pkgrecv = stat.total_pkgrecv;

	if ((stat.ts_current / 1000000) % 2 == 0)
	{
		long secc = stat.ts_current / 1000000;
		struct tm tmp = *localtime(&secc);
		if (tl_last != 0)
		{
			if (frec == NULL)
				frec = fopen("svpn.log", "a");
			printf("Rec File %08x \n\n", frec);
			fprintf(frec, "%d.%02d.%02d %02d:%02d:%02d  %9lld\n",
							tmp.tm_year, tmp.tm_mon, tmp.tm_mday,
							tmp.tm_hour, tmp.tm_min, tmp.tm_sec,
							stat.last_recv - tl_last);
			fflush(frec);
		}
		tl_last = stat.last_recv;
	}

}

int svpn_server_handle_thread(struct svpn_server* pvoid)
{
	struct svpn_server *psc = pvoid;

	struct sockaddr_in6 addr;
	socklen_t alen = sizeof(addr);

	struct timeval tv;

	char tmpstr[32];
	struct svpn_net_ipv4_header* pheader = NULL;
	unsigned char buffer[BUFFER_LEN], tmp_buffer[BUFFER_LEN];
	int ret, uid, len;
	fd_set fd_list;
	int maxfd = (psc->sock_fd > psc->tun_fd) ? psc->sock_fd : psc->tun_fd;
	maxfd++;

	strcpy(fmtstr, "Send:99,000,000,000B [9,000,000B/s], Recv:99,000,000,000B [9,000,000B/s]\r");

	memset(&stat, 0, sizeof(stat));
	gettimeofday(&tv, NULL);
	stat.ts_last = tv.tv_sec * 1000000LL + tv.tv_usec;

	while(1)
	{
		FD_ZERO(&fd_list);
		FD_SET(psc->tun_fd, &fd_list);
		FD_SET(psc->sock_fd, &fd_list);
		
		ret = select(maxfd, &fd_list, NULL, NULL, NULL);
		if(ret < 0)
		{
			if(errno == EINTR)
				return 0;
			continue;
		}

		// update statistics data
		gettimeofday(&tv, NULL);
		stat.ts_current = tv.tv_sec * 1000000LL + tv.tv_usec;


		if(FD_ISSET(psc->sock_fd, &fd_list))
		{
			len = recvfrom(psc->sock_fd, tmp_buffer, BUFFER_LEN, 0, 
				(struct sockaddr*)&addr, &alen);

			if(len <= 0 || len > BUFFER_LEN)
				continue;

			uid = tmp_buffer[0];
			if (psc->clients[uid] == NULL)
			{
				mprintf(LWARN, "Unknown user #%d", uid);
				continue;
			}

			Decrypt(&(psc->clients[uid]->table), tmp_buffer, buffer, len);

			pheader = (struct svpn_net_ipv4_header*)buffer;
			if (pheader->src_ip[0] != psc->local_addr[0] ||
				pheader->src_ip[1] != psc->local_addr[1] ||
				pheader->src_ip[2] != psc->local_addr[2] ||
				pheader->src_ip[3] != uid)
			{
				mprintf(LWARN, "Invalid password : %s", inet_ntop(PF_INET6, (char*)&addr + 8, tmpstr, sizeof(tmpstr)));
				continue;
			}

			if (memcmp(&addr, &psc->clients[uid]->addr, sizeof(addr)) != 0)
			{
				memcpy(&psc->clients[uid]->addr, &addr, sizeof(addr));
				mprintf(LINFO, "Client #%d move to %s", uid,
						inet_ntop(PF_INET6, (char*)&addr + 8, tmpstr, sizeof(tmpstr)));
			}

			stat.total_send += len;
			stat.total_pkgsend++;
			output_info();

			len = write(psc->tun_fd, buffer, len);
		}

		if(FD_ISSET(psc->tun_fd, &fd_list))
		{
			len = read(psc->tun_fd, tmp_buffer, BUFFER_LEN);

			if (len <= 0 || len > BUFFER_LEN)
				continue;

			pheader = (struct svpn_net_ipv4_header*)tmp_buffer;

			uid = pheader->dst_ip[3];
			if (psc->clients[uid] == NULL)
			{
				mprintf(LWARN, "User #%d not exist", uid);
				continue;
			}

			Encrypt(&(psc->clients[uid]->table), tmp_buffer, buffer, len);

			len = sendto(psc->sock_fd, buffer, len, 0,
					(struct sockaddr*)&psc->clients[uid]->addr,
					sizeof(struct sockaddr_in6));

			if (len <= 0)
			{
				mprintf(LERROR, "Client #%d disconnected", uid);
				continue;
			}

			// update statistics data
			stat.total_recv += len;
			stat.total_pkgrecv++;
			output_info();
		}
	}
	return 0;
}

struct svpn_server *svpn_server_init(const char* configfile)
{
	struct svpn_server *psc;
	struct sigaction sact;
	FILE* fin;
	char tmpstr[128];
	unsigned short port;
	int laddr;

	if ((fin = fopen(configfile, "r")) == NULL)
		return NULL;

	port = 0;
	laddr = 0;
	while (1)
	{
		if (fscanf(fin, "%s", tmpstr) <= 0)
			break;
		if (strcasecmp(tmpstr, "port") == 0)
			fscanf(fin, "%hd", &port);
		else if (strcasecmp(tmpstr, "local") == 0)
		{
			fscanf(fin, "%s", tmpstr);
			laddr = inet_addr(tmpstr);
		}

skipline:
		fgets(tmpstr, sizeof(tmpstr), fin);
	}
	fclose(fin);

	if (port == 0)
	{
		mprintf(LFATAL, "No port specified");
		return NULL;
	}
	if (laddr == 0)
	{
		mprintf(LFATAL, "No local address specified");
		return NULL;
	}

	psc = (struct svpn_server*)malloc(sizeof(struct svpn_server));
	memset(&sact, 0, sizeof(struct sigaction));
	memset(psc, 0, sizeof(struct svpn_server));

	memcpy(psc->local_addr, &laddr, sizeof(laddr));
	mprintf(LINFO, "Assigned local address %d.%d.%d.%d",
				psc->local_addr[0], psc->local_addr[1],
				psc->local_addr[2], psc->local_addr[3]);

	// init socket
	if(svpn_sock_create(psc, port) < 0)
	{
		free(psc);
		return NULL;
	}

	mprintf(LINFO, "Bind Server Port %d", port);

	// init tunnel
	psc->tun_fd = svpn_tun_create(psc->dev_name, laddr);
	if(psc->tun_fd < 0)
	{
		close(psc->sock_fd);
		free(psc);
		return NULL;
	}

	sact.sa_handler = svpn_sig_handler;
	sact.sa_flags &= ~SA_RESTART;
	sigaction(SIGUSR1, &sact, &(psc->old_act));

	return psc;
}

int svpn_server_init_client(struct svpn_server *psc, const char* userlist)
{
	FILE* fin;
	char tmpstr[128];
	unsigned char pmd5[16];
	int tmpint, id;

	if((fin = fopen(userlist, "r")) == NULL)
		return -1;

	memset(psc->clients, 0, sizeof(psc->clients));

	while (1)
	{
		if (fscanf(fin, "%s", tmpstr) <= 0)
			break;
		if (strcasecmp(tmpstr, "user") != 0)
			goto skipline;

		// read id
		if (fscanf(fin, "%d", &id) <= 0)
			goto skipline;
		if (psc->clients[id] != NULL)
		{
			mprintf(LERROR, "User ID duplicated");
			goto skipline;
		}
		// read password
		if (fscanf(fin, "%s", tmpstr) <= 0)
			goto skipline;
		// OK
		psc->clients[id] = 
			(struct svpn_client_node*)malloc(sizeof(struct svpn_client_node));
		MD5Fast(tmpstr, strlen(tmpstr), pmd5);
		BuildTable(&(psc->clients[id]->table), pmd5, id);

		mprintf(LINFO, "User #%d added", id);

skipline:
		fgets(tmpstr, sizeof(tmpstr), fin);
	}

	fclose(fin);
	return 0;
}
