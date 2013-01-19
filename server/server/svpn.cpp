#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include "server.h"
#include "../xml/rapidxml.hpp"
#include "../xml/rapidxml_utils.hpp"

// check source
// forwarding

using namespace std;

struct svpn_server *psc;

int main(int argc, char** argv)
{
	int ret;
	psc = NULL;
//	unsigned char pmd[16];
//	MD5Fast("a", 1, pmd);
	rapidxml::xml_document<> configdoc;
	{
		// load from file
		rapidxml::file<> fconfig("/home/hu/S-VPN/AuthedServerPlus/config.xml");
		configdoc.parse<0>(fconfig.data());
	}

	ret = svpn_server_init(configdoc.first_node());

	// dirty config hack: FIXME:
	system("ifconfig tun2 up");
	system("ifconfig tun2 192.168.3.1/24");

	ret = svpn_server_main();

	ret = svpn_server_release();

	return ret;
}


//	svpn_server_add_client(psc, 3, "59.66.133.36", pmd, 0);
//	svpn_server_add_client(psc, 6, "59.66.133.42", pmd, 0);
