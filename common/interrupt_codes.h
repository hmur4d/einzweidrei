#ifndef _INTERRUPT_CODES_H_
#define _INTERRUPT_CODES_H_

//when too many interrupts are happening and the userspace cannot process them fast enough
//this "fake" interrupt is sent instead.
#define INTERRUPT_FAILURE					  0xFF	

#define INTERRUPT_SCAN_DONE                   0x1
#define INTERRUPT_SEQUENCE_DONE               0x2
#define INTERRUPT_OVERFLOW                    0x3
#define INTERRUPT_SETUP                       0x4
#define INTERRUPT_DUMMYSCAN_DONE              0x5
#define INTERRUPT_PRESCAN_DONE                0x6
#define INTERRUPT_ACQUISITION_HALF_FULL       0x7
#define INTERRUPT_ACQUISITION_FULL            0x8
#define INTERRUPT_ACQUISITION_CORRUPTED       0x9	
#define INTERRUPT_TIME_TO_UPDATE              0xA

#define INTERRUPT_LOCK_ACQUISITION_HALF_FULL       0xB	
#define INTERRUPT_LOCK_ACQUISITION_FULL            0xC	

#endif

