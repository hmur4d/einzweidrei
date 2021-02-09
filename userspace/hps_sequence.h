#ifndef _HPS_SEQUENCE_H_
#define _HPS_SEQUENCE_H_

#include <stdint.h>   //for uint32_t etc
#include <stdbool.h>  //true
#include <stdio.h>    //printf()
#include <unistd.h>   //close()
#include <fcntl.h>    //open()
#include <sys/mman.h> //mmap()

//#define HPS_OCR_ADDRESS         0xFFE00000 
//#define HPS_OCR_SPAN            0x200000            //span in bytes

#define HPS_RESERVED_ADDRESS    0x00000000 
#define HPS_RESERVED_SPAN       0x20000000     //span in bytes

#define HPS_OCR_ADDRESS			0x20000000 
#define HPS_OCR_SPAN			0x20000000     //span in bytes

#define STEP_32b_RAM            131072



#define NB_PS     0
#define NB_DS     0
#define NB_1D     1
#define NB_2D     5
#define NB_3D     1
#define NB_4D     1

uint32_t create_events(void);

#endif