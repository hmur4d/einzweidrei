#ifndef _FPGA_BRIDGES_
#define _FPGA_BRIDGES_

/*
HPS 2 FPGA Bridge
*/

#include "linux_includes.h"

//Enables all hps>fpga, fpga>hps and fpga>sdram bridges (put their reset bit to 0)
bool enable_fpga_bridges(void);

#endif
