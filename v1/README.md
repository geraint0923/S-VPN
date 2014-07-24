S-VPN is a simple stupid VPN without any denpendency, could be easily set up
on Linux. 

Just for fun, dont't despise us, thank you!

I have tried it on my TP-Link MR11U, it works fine.

# Get and Make

1. Clone the repository from Github

		git clone git@github.com:geraint0923/S-VPN.git

2. Get into the S-VPN' s directory

		cd S-VPN/

3. Make a directory for build and get into it

		mkdir build  && cd build/

4. Build

		cmake .. && make


#Configuration

##Client
No Configuration needed, just run the command:

	sclient IP port ID PWD

There will be a tunnel named tunX being set up, you could route the packet as
you like.

Note: please the route rule to your server manually, like:

	route add -net X.X.X.X(your server's IP) gw 192.168.3.1

or

	route add -net X.X.X.X dev tunX(the tunnel used by VPN)


##Server
Need a config file, like config.example:

	port 123
	local 192.168.3.1
	user 6 a

Try the following steps:

1. the first line indicate the port used
2. the second line indicate the address of server in the subnet
3. the third line indicate the user id which must range from 3 to 240(for
   example 6), and the password(plain text)

then run the following command:

	sserver config.example(your config file name)

here a tunnel named tunX will be set up, you may need more packet forward
rules operations, such as enabling IP forwarding in Linux, NAT, etc. 

Some useful scripts could be found in scripts/ diretory.
