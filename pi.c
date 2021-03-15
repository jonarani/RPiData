#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "am2302.h"
#include "socketServer.h"

#define WAIT_TO_BE_CONSUMED 0x00
#define CONSUMED 0x01

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int FLAG = CONSUMED;
static double temperature;
static double humidity;
static int raw;
static int pin = 4;

extern int cfd;

void *produceData()
{
    // if new data is produced then signal
    // to sendData that now it can be sent (dont need to send olddata)
    for(;;)
    {
        pthread_mutex_lock(&mtx);
        
        while(FLAG == WAIT_TO_BE_CONSUMED)
        {
            pthread_cond_wait(&cond, &mtx);
        }
        
        while(pi_dht_read(pin, &humidity, &temperature, &raw) != 0)
        {
        }
        FLAG = WAIT_TO_BE_CONSUMED;        
        pthread_mutex_unlock(&mtx);

        sleep(1);
    }
}

void *sendData()
{
    int ret;                            // TODO: check pthread function ret values
    char tempBuf[30] = {0};
    char dataSendBuf[30];
    char receivedData[30];
    
    sleep(1);                           // Give a head start to producer

    recv(cfd, receivedData, 30, 0);     // read(cfd, receivedData, 30);
    printf("Read: %s\n", receivedData);

    for(;;)
    {
        pthread_mutex_lock(&mtx);
        
        printf("%f %f\n", temperature, humidity);

        gcvt(temperature, 4, tempBuf);
        strncpy(dataSendBuf, tempBuf, strlen(tempBuf));
        gcvt(humidity, 5, tempBuf);
        strcat(dataSendBuf, "|");
        strncat(dataSendBuf, tempBuf, strlen(tempBuf));
        
        ssize_t ret = send(cfd, dataSendBuf, strlen(dataSendBuf), 0);
        memset(dataSendBuf, 0, sizeof(dataSendBuf));

        FLAG = CONSUMED;
        pthread_mutex_unlock(&mtx);
        pthread_cond_signal(&cond);

        sleep(1);
    }
}

int main(void)
{
    if (!bcm2835_init())
    {
        printf("Init failed\n");
        return EXIT_FAILURE;
    }

    pthread_t t1;
    pthread_t t2;
    struct sockaddr_in serveraddr, clientaddr;

    pthread_create(&t1, NULL, produceData, NULL);
    pthread_create(&t2, NULL, sendData, NULL);
    initSocket(&serveraddr, &clientaddr);

    printf("Init completed\n");

    char *client_ip = inet_ntoa(clientaddr.sin_addr);
    int client_port = ntohs(clientaddr.sin_port);
    printf("client ip: %s\n port: %d\n", client_ip, client_port);

    sleep(1);

    for(;;)
    {  

    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    closeSockets();
    
    bcm2835_close();

    return 0;
}