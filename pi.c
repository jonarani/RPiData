#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "am2302.h"
#include "socketServer.h"

#define NEW_DATA_READY 0x00
#define DATA_SENT 0x01

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int FLAG = DATA_SENT;
static double temperature;
static double humidity;
static int raw;
static int pin = 4;

void *produceData();
void *sendData(void *cfd);

int main(void)
{
    if (!bcm2835_init())
    {
        printf("Init failed\n");
        return EXIT_FAILURE;
    }

    int cfd, sfd;
    struct sockaddr_in serveraddr, clientaddr;
    initSocket(&serveraddr, &clientaddr, &sfd, &cfd);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, produceData, NULL);
    pthread_create(&t2, NULL, sendData, &cfd);

    printf("Init completed\n");

    char *client_ip = inet_ntoa(clientaddr.sin_addr);
    int client_port = ntohs(clientaddr.sin_port);
    printf("client ip: %s\n port: %d\n", client_ip, client_port);

    for(;;)
    {  
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    close(sfd);
    close(cfd);
    bcm2835_close();

    return 0;
}

void *produceData()
{
    for(;;)
    {
        pthread_mutex_lock(&mtx);
        
        while(pi_dht_read(pin, &humidity, &temperature, &raw) != 0){}
        FLAG = NEW_DATA_READY;        
        
        pthread_mutex_unlock(&mtx);
        pthread_cond_signal(&cond);

        sleep(1);
    }
}

void *sendData(void *fd)
{
    char tempBuf[30] = {0};
    char dataSendBuf[30];
    // char receivedData[30];
    int cfd = *(int *)fd;
    
    sleep(1);   // Give a head start to producer

    // recv(cfd, receivedData, 30, 0);  // or read(cfd, receivedData, 30);
    // printf("Read: %s\n", receivedData);

    for(;;)
    {
        pthread_mutex_lock(&mtx);
        while(FLAG != NEW_DATA_READY)
        {
            pthread_cond_wait(&cond, &mtx);
        }
        
        printf("%f %f\n", temperature, humidity);

        gcvt(temperature, 4, tempBuf);
        strncpy(dataSendBuf, tempBuf, strlen(tempBuf));
        gcvt(humidity, 5, tempBuf);
        strcat(dataSendBuf, "|");
        strncat(dataSendBuf, tempBuf, strlen(tempBuf));
        
        send(cfd, dataSendBuf, strlen(dataSendBuf), 0);
        memset(dataSendBuf, 0, sizeof(dataSendBuf));

        FLAG = DATA_SENT;
        pthread_mutex_unlock(&mtx);
    }
}