#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "am2302.h"

#define WAIT_TO_BE_CONSUMED 0x00
#define CONSUMED 0x01

#define PORT_NUM 47777

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int FLAG = CONSUMED;
static double temperature;
static double humidity;
static int raw;
static int pin = 4;

static int sfd;
static int cfd;

void *produceData()
{
    int ret;    // TODO: check pthread function ret values
                //       if new data is produced then signal
                //       to sendData that now it can be sent (dont need to send olddata)
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

    int ret;
    pthread_t t1;
    pthread_t t2;

    struct sockaddr_in serveraddr, clientaddr;
    int opt = 1;
    int addrlen = sizeof(struct sockaddr_in);

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    // if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    // {
    //     printf("setsockopt() failed.\n");
    //     exit(EXIT_FAILURE);
    // }   

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT_NUM);

    if(bind(sfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("bind() failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(sfd, 3) < 0)
    {
        printf("listen() failed.\n");
        exit(EXIT_FAILURE);
    }

    if((cfd = accept(sfd, (struct sockaddr *)&clientaddr, (socklen_t*)&addrlen)) < 0)
    {
        printf("accept() failed.\n");
        exit(EXIT_FAILURE);
    }
    
    ret = pthread_create(&t1, NULL, produceData, NULL);
    ret = pthread_create(&t2, NULL, sendData, NULL);

    printf("Init completed\n");

    char *client_ip = inet_ntoa(clientaddr.sin_addr);
    int client_port = ntohs(clientaddr.sin_port);
    printf("client ip: %s\n port: %d\n", client_ip, client_port);

    sleep(1);

    for(;;)
    {  

    }

    ret = pthread_join(t1, NULL);
    ret = pthread_join(t2, NULL);
    
    close(sfd);
    close(cfd);
    
    bcm2835_close();

    return 0;
}

// LED for debugging
// bcm2835_gpio_fsel(11, BCM2835_GPIO_FSEL_OUTP);
// bcm2835_gpio_write(11, LOW);