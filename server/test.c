#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "crypt.h"

int main() {
	unsigned char buffer[4096], tmp_buffer[4096], src_ip[4], dst_ip[4];
	int sock;
	struct sockaddr_in saddr, caddr;
	struct CodeTable table;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(33333);
	memset(saddr.sin_zero, 0, sizeof(saddr.sin_zero));

	if(bind(sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr))) {
		perror("bind");
		return 1;
	}

	BuildTable(&table, "a", 0);

	int clen = sizeof(caddr);
	while(1) {
		int len = recvfrom(sock, buffer, 4096, 0, (struct sockaddr*)&caddr, &clen);
		if(len < 0) {
			perror("recvfrom");
			continue;
		}
		// decrypt
		Decrypt(&table, buffer, tmp_buffer, len);
//		memcpy(tmp_buffer, buffer, len);

		//change ip address
		memcpy(src_ip, &tmp_buffer[12], 4);
		memcpy(dst_ip, &tmp_buffer[16], 4);
		memcpy(&tmp_buffer[12], &tmp_buffer[16], 4);
		memcpy(&tmp_buffer[16], src_ip, 4);


		tmp_buffer[20] = 0;
		*((unsigned short*)&tmp_buffer[22]) += 8;

		printf("%d.%d.%d.%d  ->  %d.%d.%d.%d\n",
				src_ip[0], src_ip[1], src_ip[2], src_ip[3],
				dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);


//		//encrypt
		Encrypt(&table, tmp_buffer, buffer, len);
//		memcpy(buffer, tmp_buffer, len);

		len = sendto(sock, buffer, len, 0, (struct sockaddr*)&caddr, clen);
		if(len < 0) {
			perror("sendto");
		}
		printf("sendto : %d bytes\n", len);
	}
	return 0;
}
