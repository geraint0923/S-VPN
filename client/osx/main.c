#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_LEN	4096

int tun_create(char *dev) {
	int fd, err;
	
	if((fd = open(dev, O_RDWR)) < 0) {
		perror("tun_create");
		return fd;
	}

	return fd;
}


int main() {
	int fd = tun_create("/dev/tun0");
	char buffer[BUFFER_LEN];
	if(fd < 0) {
		return -1;
	}
	system("ifconfig tun0 up");
	system("ifconfig tun0 192.168.3.3 192.168.3.1");
	while(1) {
		unsigned char src_ip[4];
		unsigned char dst_ip[4];
		int len = read(fd, buffer, BUFFER_LEN);
		printf("I received %d bytes!\n", len);
		if(len <= 0) {
			printf("fuck?\n");
			continue;
		}
		memcpy(src_ip, &buffer[12], 4);
		memcpy(dst_ip, &buffer[16], 4);
		memcpy(&buffer[12], &buffer[16], 4);
		memcpy(&buffer[16], src_ip, 4);
		buffer[20] = 0;
		*((unsigned short*)&buffer[22]) += 8;
		printf("from %d.%d.%d.%d -> to %d.%d.%d.%d\n", 
				src_ip[0], src_ip[1], src_ip[2], src_ip[3],
				dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
		len = write(fd, buffer, len);
		printf("I sent %d bytes!\n", len);
	}
	return 0;
}
