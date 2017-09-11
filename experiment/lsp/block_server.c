#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE		1024
#define OPSZ			4

void error_handler(char *msg);
int calculate(int opnum, int opnds[], char op);

int main(int argc, char *argv[])
{
        int serv_sock;
        int clnt_sock;
	char opinfo[BUF_SIZE];
	int res, opnd_cnt, i;
	int recv_cnt, recv_len;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_sz;

	if(argc != 2)
	{
		printf("Usage: %s port\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == 1)
		error_handler("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handler("bind() error");

	if(listen(serv_sock, 1) == -1)
		error_handler("listen() error");

	clnt_addr_sz = sizeof(clnt_addr);

	opnd_cnt = 0;
	clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_sz);
	read(clnt_sock, &opnd_cnt, 1);

	recv_len = 0;
	while((opnd_cnt * OPSZ + 1) > recv_len)
	{
		recv_cnt = read(clnt_sock, &opinfo[recv_len], BUF_SIZE - 1);
		recv_len += recv_cnt;
	}

	res = calculate(opnd_cnt, (int *)opinfo, opinfo[recv_len - 1]);
	write(clnt_sock, (char *)&res, sizeof(res));
	close(clnt_sock);
	close(serv_sock);

	return 0;
}

int calculate(int opnum, int opnds[], char op)
{
	int res = opnds[0], i;

	switch(op)
	{
		case '+':
			for(i = 1; i < opnum; i++)
				res += opnds[i];
			break;
		case '-':
			for(i = 1; i < opnum; i++)
				res -= opnds[i];
			break;
		case '*':
			for(i = 1; i < opnum; i++)
				res *= opnds[i];
			break;
	}

	return res;
}

void error_handler(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
