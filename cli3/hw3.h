#ifndef HW1_H
#define HW1_H
#define MAXLINE 2048
#define LISTENQ 1024
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <pthread.h>

struct  L
{
	int connfd;
	struct sockaddr_in ip;
} typedef login;

struct U{
	char name[20];
	char file[MAXLINE];
	int file_num;
	int sock;
	char ip[MAXLINE];
} typedef user;

struct F{
	char ip[MAXLINE];
	char file[MAXLINE];
	int offset;
	int chunk;
} typedef getFile;

struct T {
	char file[20];
	int size; 
} typedef files;

#endif
