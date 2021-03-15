#pragma

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT_NUM 47777

void initSocket(struct sockaddr_in *serveraddr, struct sockaddr_in *clientaddr);

void closeSockets();