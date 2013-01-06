#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "config.h"
#include "Client.h"

int main(int argc, char** argv)
{
	if (argc == 0)
	{
		printf("SVPN Windows Client, Version Alpha 0.1.0\n");
		printf("Copyright Yangyang, Huxiaoxiang\n");
		printf("\n");
		printf("Tap-Win32 Driver (MUST BE V9.9) is required.\n");
		printf("It can be installed with newest OpenVPN, or individually.\n");
		printf("\n");
		printf("Parameters:\n");
		printf("argv[1] : Tun IP address, e.g. 192.168.3.6\n");
		printf("argv[2] : Server name or IP, e.g. www.huxx.me\n");
		printf("argv[3] : Server UDP port, e.g. 33333\n");
		printf("Currently username and password is immutable.\n");
		return 0;
	}
	Config cfg;
	cfg.UseCommandLineConfig(argv);
	Client cl;
	cl.Initialize(cfg);
	cl.Run();
	cl.Finalize();
}
