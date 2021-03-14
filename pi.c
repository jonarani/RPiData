#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "am2302.h"

#define WAIT_TO_BE_CONSUMED 0x00
#define CONSUMED 0x01

#define PORT_NUM 50000

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int FLAG = CONSUMED;
static float temperature;
static float humidity;
static int pin = 4;

static int sfd;
static int cfd;

void *produceData()
{
    int ret;    // TODO: check pthread function ret values
    for(;;)
    {
        pthread_mutex_lock(&mtx);
        
        while(FLAG == WAIT_TO_BE_CONSUMED)
        {
            pthread_cond_wait(&cond, &mtx);
        }
        
        while(pi_dht_read(pin, &humidity, &temperature) != 0)
        {
        }
        FLAG = WAIT_TO_BE_CONSUMED;        
        pthread_mutex_unlock(&mtx);

        sleep(10);
    }
}

void *sendData()
{
    int ret;    // TODO: check pthread function ret values
    char receivedData[30];
    int valRead = 0;
    
    sleep(1);   // Give a head start to producer

    for(;;)
    {
        // valRead = read(cfd, receivedData, 30);
        // printf("Read from socket: %d - %s\n", valRead, receivedData);
        // TODO: Send data to the client or server

        ssize_t ret = send(cfd, "Random data", strlen("Random data"), 0);
        printf("ret: %u\n", ret);

        pthread_mutex_lock(&mtx);
        
        printf("%f %f\n", temperature, humidity);

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

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("setsockopt() failed.\n");
        exit(EXIT_FAILURE);
    }   

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_NUM);

    if(bind(sfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("bind() failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(sfd, 3) < 0)
    {
        printf("listen() failed.\n");
        exit(EXIT_FAILURE);
    }

    if((cfd = accept(sfd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
    {
        printf("accept() failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected?\n");
    
    ret = pthread_create(&t1, NULL, produceData, NULL);
    ret = pthread_create(&t2, NULL, sendData, NULL);

    printf("Init completed\n");

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