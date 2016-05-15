#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

static int serv_sock;
static int clnt_sock;
static int clnt_addr_size;

struct sockaddr_in serv_addr;
struct sockaddr_in clnt_addr;

void error_handling(char *message);
void init_server(int port);
void connect_client();
void send_to_client(char *data, int len);

void init_server(int port)
{
	serv_sock = socket(PF_INET, SOCK_STREAM,0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))== -1)
		error_handling("bind() error");

	if (listen(serv_sock, 2) == -1)
		error_handling("listen() error");
}

void connect_client()
{
	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

	printf("new connect, client ip : %s\n", inet_ntoa(clnt_addr.sin_addr));
	printf("a new client file discriptor : %d\n", clnt_sock);
}

void send_to_client(char *data, int len)
{
	write(clnt_sock, data, len);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

#endif
