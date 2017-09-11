#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE		1024

void error_handler(char *msg);

int main(int argc, char **argv)
{
	int sock;
	char msg[BUF_SIZE];
	int str_len, recv_len, recv_cnt;
	struct sockaddr_in serv_addr;

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
		error_handler("connect() error!");
	else
		puts("Connected ......");

	for(;;)
	{
		fputs("Input msg(q to quit): ", stdout);
		fgets(msg, BUF_SIZE, stdin);
		if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
			break;

		str_len = write(sock, msg, strlen(msg));

		recv_len = 0;
		while(recv_len < str_len)
		{
			recv_cnt = read(sock, &msg[recv_len], BUF_SIZE - 1);
			if(recv_cnt == -1)
				error_handler("read() error!");
			recv_len += recv_cnt;
		}
		msg[recv_len] = 0;
		printf("msg from server: %s\n", msg);
	}

	close(sock);

	return 0;
}

void error_handler(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
