#include "socketServer.h"

void initSocket(struct sockaddr_in *serveraddr, struct sockaddr_in *clientaddr, int *sfd, int *cfd)
{
    int addrlen = sizeof(struct sockaddr_in);
    if((*sfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    serveraddr->sin_family = AF_INET;
    serveraddr->sin_addr.s_addr = INADDR_ANY;
    serveraddr->sin_port = htons(PORT_NUM);

    if(bind(*sfd, (struct sockaddr *)serveraddr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("bind() failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(*sfd, 3) < 0)
    {
        printf("listen() failed.\n");
        exit(EXIT_FAILURE);
    }

    if((*cfd = accept(*sfd, (struct sockaddr *)clientaddr, (socklen_t*)&addrlen)) < 0)    // Block until client makes contact
    {
        printf("accept() failed.\n");
        exit(EXIT_FAILURE);
    }
}