#ifndef _FPGA_DMA_H_
#define _FPGA_DMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>


int transfer_to_fpga(uint32_t nb_of_events);

#endif