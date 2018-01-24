#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>
#include "../common/interrupts.h"

#define MEM_FILE "/dev/mem"
#define INTERRUPTS_FILE "/dev/interrupts"

typedef struct {
    void (*button_pressed) (void *);
} handlers_t;

typedef struct {
    int fd;
    pthread_t thread;
    handlers_t handlers;
} interrupts_t;

typedef struct {
    interrupts_t interrupts;
    
    //read/write memory map
    int devmem_fd;
    void* lw_bridge;    
    uint32_t* leds;
} device_t;


/**
 * Load ressources, map memory access and start interruption handling thread.
 */
int device_open(device_t *device);

int device_close(device_t *device);

#endif /* _DEVICE_H_ */

