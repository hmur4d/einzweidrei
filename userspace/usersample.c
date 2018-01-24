/*
 * Exemple de structuration de programme userspace avec abstraction du hardware 
 * dans un "module" device.
 * 
 * Lors de l'interruption, change le mode de clignottement pour montrer les 
 * interactions possibles avec un autre thread.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>

#include "addresses.h"
#include "device.h"

static __useconds_t sleep_time = 1000000;

void button_pressed(void *arg) {
    char interrupt = *(char*)arg;
    printf("button pressed: %d\n", (int)interrupt);
    
    if(sleep_time < 50000)
        sleep_time = 1000000;
    else
        sleep_time /= 4;
}

void* blink_thread(void* arg) {
    device_t *device = (device_t*)arg;

    while(1) {
        *device->leds = *device->leds ? 0x00: 0xFF;
        usleep(sleep_time);
    }
    
    pthread_exit (0);
}

int disabled_main(int argc, char ** argv) {
    device_t device;
    device.interrupts.handlers.button_pressed = button_pressed;

    if(device_open(&device) != 0) {
        fprintf(stderr, "error mapping hardware\n");
        exit(1);
    }
    
    pthread_t blinker;
    if (pthread_create(&blinker, NULL, blink_thread, &device) < 0) {
        fprintf (stderr, "pthread_create error\n");
        exit(1);
    }
  
    printf("press enter to exit\n");
    getchar();
    
    printf("canceling blinker\n");
    pthread_cancel(blinker);
    pthread_join(blinker, NULL);
    printf("threads joined, closing device\n");
    
    device_close(&device);
    printf("done.\n");
    
    return 0;
}
