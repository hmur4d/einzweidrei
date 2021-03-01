#ifndef _HPS_SEQUENCE_GRAD_H_
#define _HPS_SEQUENCE_GRAD_H_

#include <stdint.h>   //for uint32_t etc
#include <stdbool.h>  //true
#include <stdio.h>    //printf()
#include <unistd.h>   //close()
#include <fcntl.h>    //open()
#include <sys/mman.h> //mmap()

#define HPS_RESERVED_ADDRESS    1073741824 
#define HPS_RESERVED_SPAN       (524288000)     //500Megabytes

#define STEP_32b_RAM            131072

uint32_t create_events_grad(void);

#endif