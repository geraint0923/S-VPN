#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "crypt.h"
#include "svpn_server.h"

// check source
// forwarding

int main() {
	struct svpn_server *psc = NULL;
//	unsigned char pmd[16];
//	MD5Fast("a", 1, pmd);

	psc = svpn_server_init();

//	svpn_server_add_client(psc, 3, "59.66.133.36", pmd, 0);
//	svpn_server_add_client(psc, 6, "59.66.133.42", pmd, 0);

//	system("ifconfig tun2 up");
//	system("ifconfig tun2 192.168.3.1/24");

	svpn_server_main(psc);

	svpn_server_release(psc);

	return 0;
}
