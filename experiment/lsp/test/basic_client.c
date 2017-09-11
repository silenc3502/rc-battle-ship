#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handler(char *msg);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	char msg[30];
	int str_len;

	if(argc != 3)
	{
		printf("Usage: %s IP port\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if(sock == -1)
		error_handler("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handler("connect() error");

	str_len = read(sock, msg, sizeof(msg) - 1);
	if(str_len == -1)
		error_handler("read() error");

	printf("msg from server: %s\n", msg);
	close(sock);

	return 0;
}

void error_handler(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
