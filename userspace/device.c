#ifdef INTELLISENSE
    #include <asm-generic/mman-common.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>

#include "device.h"
#include "addresses.h"


void* interrupts_handler_thread(void* arg) {
    interrupts_t *interrupts = (interrupts_t*)arg;
    
    char c;
    while(read(interrupts->fd, &c, 1) == 1) {
        switch(c) {
            case BUTTON_INTERRUPT:
                if(interrupts->handlers.button_pressed) {
                    interrupts->handlers.button_pressed(&c);
                }
                break;
            default:
                fprintf(stderr, "unknown interrupt: %d\n", c);
        }
    }
    
    fprintf(stderr, "stopped reading interrupts, read failed");
    pthread_exit (0);
}

int create_interrupts_reader_thread(interrupts_t *interrupts) {
    return pthread_create(&(interrupts->thread), NULL, interrupts_handler_thread, interrupts);
}

int device_open(device_t *device) {
    //open device file for interrupts
    device->interrupts.fd = open(INTERRUPTS_FILE, O_RDONLY);
    if(device->interrupts.fd < 0) {
        perror("open /dev/interrupts");
        return device->interrupts.fd;
    }
    
    //start interrupt thread
    if(create_interrupts_reader_thread(&(device->interrupts)) < 0) {
        perror("error starting interrupt reader thread");
        return -1;
    }
    
    //open ram
    device->devmem_fd = open(MEM_FILE, O_RDWR | O_SYNC);
    if(device->devmem_fd < 0) {
        perror("open /dev/mem");
        device_close(device);
        return device->devmem_fd;
    }

    // mmap() the entire address space of the Lightweight bridge so we can access our custom module 
    device->lw_bridge = (uint32_t*)mmap(NULL, HPS_TO_FPGA_LW_SPAN, PROT_READ|PROT_WRITE, MAP_SHARED, device->devmem_fd, HPS_TO_FPGA_LW_BASE); 
    if(device->lw_bridge == MAP_FAILED) {
        perror("mmap lw_brigde on /dev/mem");
        device_close(device);
        return -1;
    }

    //assign memory blocks
    device->leds = (uint32_t*)(device->lw_bridge + LEDS_BASE);
    return 0;
} 

int device_close(device_t *device) {
    //XXX error handling

    pthread_cancel(device->interrupts.thread);
    pthread_join(device->interrupts.thread, NULL);
    
    if(device->devmem_fd >= 0) {
        if(device->lw_bridge != MAP_FAILED) {
            munmap(device->lw_bridge, HPS_TO_FPGA_LW_SPAN); 
        }
        
        close(device->devmem_fd);    
    }
    
    if(device->interrupts.fd >= 0) {
        close(device->interrupts.fd);
    }
    
    return 0;
}