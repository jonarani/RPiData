#include <stdio.h>
#include <bcm2835.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
// #include <sys/types.h>
#include "am2302.h"

#define WAIT_TO_BE_CONSUMED 0x00
#define CONSUMED 0x01

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int FLAG = CONSUMED;
static float temperature;
static float humidity;
static int pin = 4;

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
    }
}

void *consumeData()
{
    int ret;    // TODO: check pthread function ret values
    sleep(1);   // Give a head start to producer

    for(;;)
    {
        pthread_mutex_lock(&mtx);

        // TODO: Send data to the client or server


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

    bcm2835_gpio_fsel(11, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(11, LOW);

    int ret;
    pthread_t t1;
    pthread_t t2;

    ret = pthread_create(&t1, NULL, produceData, NULL);
    ret = pthread_create(&t2, NULL, consumeData, NULL);

    for(;;)
    {
        
    }

    ret = pthread_join(t1, NULL);
    ret = pthread_join(t2, NULL);
    bcm2835_close();

    return 0;
}