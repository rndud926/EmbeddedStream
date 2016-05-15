/*
 ============================================================================
 Name        : client.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>

void error_handling(char *message);

void init_client(int argc, char **argv)
{
	if (argc != 4) 
	{		printf("Usage : %s <IP> <PORT> <name> \n", argv[0]);		exit(1);	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock == -1)		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
}

void recv_server_data(char *data,int ren)
{
	read(sock,data,ren);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

#endif