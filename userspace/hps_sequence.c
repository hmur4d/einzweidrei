#include "hps_sequence.h"
#include "fpga_dmac_api.h"


//#define HPS_OCR_ADDRESS           0xFFE00000 
//#define HPS_OCR_SPAN              2097152            //span in bytes

#define DDR_EVENTS_ADDRESS			1598029824          //use upper portion of the 1GB
#define DDR_EVENTS_SPAN			    65536                //span in bytes

#define LW_BASE                     0xff200000
#define LW_SPAN                     1048576
#define FPGA_DMAC_QSYS_ADDRESS      0x00020080
#define FPGA_DMAC_ADDRESS           ((uint8_t*)LW_BASE+FPGA_DMAC_QSYS_ADDRESS)

//fifo is connected directly to dma
#define DMA_TRANSFER_DST_DMAC       0x0
#define DMA_FULL_BURST_IN_BYTES     16384 //1024*16 this is 128 event
#define DMA_TRANSFER_1_SRC_DMAC     (DDR_EVENTS_ADDRESS)
#define DMA_TRANSFER_2_SRC_DMAC     (DDR_EVENTS_ADDRESS+DMA_FULL_BURST_IN_BYTES)

//counters
#define _PS 0
#define _DS 1
#define _1D 2
#define _2D 3
#define _3D 4
#define _4D 5


uint32_t scan_counters[6] = { 0,0,0,0,0,0 };
uint32_t loopa_counters[2] = { 0,0 };

// nb of elements per counter to be used for the modulus
#define O_0  0
#define O_1D 1
#define O_2D 2
#define O_3D 3
#define O_4D 4


uint32_t nb_elements_per_counter[16]; 

uint32_t static get_addr(uint32_t* scan_counter, uint32_t base_address, uint32_t order, uint32_t nb_of_values) {
    return (scan_counter[(order==0? 0:order+1)] % nb_of_values) + base_address ;
}

uint32_t create_events(void) {
    int fd;
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return(1);
    }
    
    //access events space
    
    void* events_base    = mmap( NULL, DDR_EVENTS_SPAN, (PROT_READ | PROT_WRITE),
                            MAP_SHARED, fd, DDR_EVENTS_ADDRESS);

    if (events_base == MAP_FAILED) {
        printf("ERROR: mmap() events_base failed...\n");
        close(fd);
        return(1);
    }
    uint32_t* events_base_ptr = (uint32_t*)events_base;
    

    //access reserved ddr
    void* reserved_mem_base;
    reserved_mem_base = mmap(NULL, HPS_RESERVED_SPAN, (PROT_READ | PROT_WRITE),
        MAP_SHARED, fd, HPS_RESERVED_ADDRESS);

    if (reserved_mem_base == MAP_FAILED) {
        printf("ERROR: mmap() reserved_mem_base failed...\n");
        close(fd);
        return(1);
    }

    //mmap dmac addr
    void* lw_vaddr;
    lw_vaddr = mmap(NULL, LW_SPAN, (PROT_READ | PROT_WRITE),
        MAP_SHARED, fd, LW_BASE);

    if (lw_vaddr == MAP_FAILED) {
        printf("ERROR: mmap() lw failed...\n");
        close(fd);
        return(1);
    }

    //virtual addresses for all components
    void* FPGA_DMA_vaddr_void = (uint8_t*)lw_vaddr + FPGA_DMAC_QSYS_ADDRESS;


    printf("Initializing DMA Controller\n");
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,
        FPGA_DMA_CONTROL,
        FPGA_DMA_QUADWORD_TRANSFERS |
        FPGA_DMA_END_WHEN_LENGHT_ZERO
    );

    fpga_dma_write_reg(FPGA_DMA_vaddr_void,  //set destiny address
        FPGA_DMA_WRITEADDRESS,
        (uint32_t)DMA_TRANSFER_DST_DMAC);


    //RAMS
    uint32_t* base_rams         = (uint32_t*)reserved_mem_base;
    //event ram
    uint32_t* ram_func_ptr      = base_rams + 0  * STEP_32b_RAM;
    uint32_t* ram_ttl_ptr       = base_rams + 1  * STEP_32b_RAM;
    uint32_t* ram_orders_ptr    = base_rams + 3  * STEP_32b_RAM;
    uint32_t* ram_adr_c1_ptr    = base_rams + 4  * STEP_32b_RAM;
    uint32_t* ram_adr_c2_ptr    = base_rams + 5  * STEP_32b_RAM;
    uint32_t* ram_adr_c3_ptr    = base_rams + 6  * STEP_32b_RAM;
    uint32_t* ram_adr_c4_ptr    = base_rams + 7  * STEP_32b_RAM;
    uint32_t* ram_adr_c1b_ptr   = base_rams + 111 * STEP_32b_RAM;
    uint32_t* ram_adr_c2b_ptr   = base_rams + 112 * STEP_32b_RAM;
    uint32_t* ram_adr_c3b_ptr   = base_rams + 113 * STEP_32b_RAM;
    uint32_t* ram_adr_c4b_ptr   = base_rams + 114 * STEP_32b_RAM;


    //element ram
    uint32_t* ram_timer_ptr     = base_rams + 49 * STEP_32b_RAM;

    uint32_t* ram_freq1_ptr     = base_rams + 25 * STEP_32b_RAM;
    uint32_t* ram_phase1_ptr    = base_rams + 26 * STEP_32b_RAM;
    uint32_t* ram_amp1_ptr      = base_rams + 27 * STEP_32b_RAM;
   // uint32_t* ram_tx_shape1_ptr = base_rams + 28 * STEP_32b_RAM;
    uint32_t* ram_freq2_ptr     = base_rams + 29 * STEP_32b_RAM;
    uint32_t* ram_phase2_ptr    = base_rams + 30 * STEP_32b_RAM;
    uint32_t* ram_amp2_ptr      = base_rams + 31 * STEP_32b_RAM;
   // uint32_t* ram_tx_shape2_ptr = base_rams + 32 * STEP_32b_RAM;
    uint32_t* ram_freq3_ptr     = base_rams + 33 * STEP_32b_RAM;
    uint32_t* ram_phase3_ptr    = base_rams + 34 * STEP_32b_RAM;
    uint32_t* ram_amp3_ptr      = base_rams + 35 * STEP_32b_RAM;
   // uint32_t* ram_tx_shape3_ptr = base_rams + 36 * STEP_32b_RAM;
    uint32_t* ram_freq4_ptr     = base_rams + 37 * STEP_32b_RAM;
    uint32_t* ram_phase4_ptr    = base_rams + 38 * STEP_32b_RAM;
    uint32_t* ram_amp4_ptr      = base_rams + 39 * STEP_32b_RAM;



    uint32_t* base_of_regs = (uint32_t*)reserved_mem_base + (131072 * 87);
    uint32_t  nb_dimensions[6];
    nb_dimensions[_DS]            = *(base_of_regs + 7);
    nb_dimensions[_1D]            = *(base_of_regs + 8);
    nb_dimensions[_2D]            = *(base_of_regs + 9);
    nb_dimensions[_3D]            = *(base_of_regs + 10);
    nb_dimensions[_4D]            = *(base_of_regs + 11);
    nb_dimensions[_PS]            = *(base_of_regs + 93);

    nb_elements_per_counter[O_0]  = 1+ 0;
    nb_elements_per_counter[O_1D] = 1+ *(base_of_regs + 12);
    nb_elements_per_counter[O_2D] = 1+ *(base_of_regs + 13);
    nb_elements_per_counter[O_3D] = 1+ *(base_of_regs + 14);
    nb_elements_per_counter[O_4D] = 1+ *(base_of_regs + 15);


    uint32_t nb_of_all_events = 0;
    
    /*
    for (int i = 0; i < 5; i++) {
        printf("ram ttl %d at %x \n", i, ram_ttl_ptr[i]);
    }
    */


    for (int i = 0; i < 5; i++) {
        printf("ram_adr_c1b %d at %x \n", i, ram_adr_c1b_ptr[i]);
    }

    for (int i = 0; i < 5; i++) {
        printf("ram_amp1 %d at %x \n", i, ram_amp1_ptr[i]);
    }
    
    for (int i = 0; i < _4D; i++) {
        printf("NB scans %d at %x \n", i, nb_dimensions[i]);
    }
    
    for (int i = 0; i < O_4D; i++) {
        printf("NB elements %d at %x \n", i, nb_elements_per_counter[i]);
    }

    uint32_t nb_of_events_treated = 0; 
    bool is_second_trans = false;

    for (scan_counters[_4D] = 0; scan_counters[_4D] <= nb_dimensions[_4D]; scan_counters[_4D]++) {
        for (scan_counters[_3D] = 0; scan_counters[_3D] <= nb_dimensions[_3D]; scan_counters[_3D]++) {
            for (scan_counters[_2D] = 0; scan_counters[_2D] <= nb_dimensions[_2D]; scan_counters[_2D]++) {
                for (scan_counters[_1D] = 0; scan_counters[_1D] <= nb_dimensions[_1D]; scan_counters[_1D]++) {
                    while (true) {

                        //get the current event
                        //get the timer address : look at the timer addr and timer order
                        uint32_t timer_base_addr    = ((0x3FF   << 22) & *ram_func_ptr) >> 22;
                        uint32_t timer_order        = ((0xF     << 18) & *ram_func_ptr) >> 18;

                        uint32_t timer_addr         = get_addr(scan_counters, timer_base_addr, timer_order, nb_elements_per_counter[timer_order]);

                        //get the tx address
                        //freq
                        uint32_t  tx1_freq_base_addr     = ((0x3FF << 0 ) & *ram_adr_c1_ptr) >> 0;
                        uint32_t  tx1_freq_order         = ((0xF   << 0 ) & *ram_orders_ptr) >> 0;
                        uint32_t  tx1_freq_addr          = get_addr(scan_counters, tx1_freq_base_addr, tx1_freq_order, nb_elements_per_counter[tx1_freq_order]);
                        //phase
                        uint32_t  tx1_phase_base_addr    = ((0x3FF << 0) & *ram_adr_c1b_ptr) >> 0;
                        uint32_t  tx1_phase_order        = ((0xF   << 16) & *ram_orders_ptr) >> 16;
                        uint32_t  tx1_phase_addr         = get_addr(scan_counters, tx1_phase_base_addr, tx1_phase_order, nb_elements_per_counter[tx1_phase_order]);
                        //amp
                        uint32_t  tx1_amp_base_addr      = ((0x3FF << 11) & *ram_adr_c1b_ptr) >> 11;
                        uint32_t  tx1_amp_order          = ((0xF   << 22) & *ram_adr_c1b_ptr) >> 22;
                        uint32_t  tx1_amp_addr           = get_addr(scan_counters, tx1_amp_base_addr, tx1_amp_order, nb_elements_per_counter[tx1_amp_order]);


                        uint32_t  tx2_freq_base_addr     = ((0x3FF << 0 ) & *ram_adr_c2_ptr) >> 0;
                        uint32_t  tx2_freq_order         = ((0xF   << 4 ) & *ram_orders_ptr) >> 4;
                        uint32_t  tx2_freq_addr          = get_addr(scan_counters, tx2_freq_base_addr, tx2_freq_order, nb_elements_per_counter[tx2_freq_order]);
                        //phase
                        uint32_t  tx2_phase_base_addr    = ((0x3FF << 0) & *ram_adr_c2b_ptr) >> 0;
                        uint32_t  tx2_phase_order        = ((0xF   << 16) & *ram_orders_ptr) >> 16;
                        uint32_t  tx2_phase_addr         = get_addr(scan_counters, tx2_phase_base_addr, tx2_phase_order, nb_elements_per_counter[tx2_phase_order]);
                        //amp
                        uint32_t  tx2_amp_base_addr      = ((0x3FF << 11) & *ram_adr_c2b_ptr) >> 11;
                        uint32_t  tx2_amp_order          = ((0xF   << 22) & *ram_adr_c2b_ptr) >> 22;
                        uint32_t  tx2_amp_addr           = get_addr(scan_counters, tx2_amp_base_addr, tx2_amp_order, nb_elements_per_counter[tx2_amp_order]);
                        
                        //    
                        uint32_t  tx3_freq_base_addr     = ((0x3FF << 0 ) & *ram_adr_c3_ptr) >> 0;
                        uint32_t  tx3_freq_order         = ((0xF   << 8 ) & *ram_orders_ptr) >> 8;
                        uint32_t  tx3_freq_addr          = get_addr(scan_counters, tx3_freq_base_addr, tx3_freq_order, nb_elements_per_counter[tx3_freq_order]);
                        //phase
                        uint32_t  tx3_phase_base_addr    = ((0x3FF << 0) & *ram_adr_c3b_ptr) >> 0;
                        uint32_t  tx3_phase_order        = ((0xF   << 16) & *ram_orders_ptr) >> 16;
                        uint32_t  tx3_phase_addr         = get_addr(scan_counters, tx3_phase_base_addr, tx3_phase_order, nb_elements_per_counter[tx3_phase_order]);
                        //amp
                        uint32_t  tx3_amp_base_addr      = ((0x3FF << 11) & *ram_adr_c3b_ptr) >> 11;
                        uint32_t  tx3_amp_order          = ((0xF   << 22) & *ram_adr_c3b_ptr) >> 22;
                        uint32_t  tx3_amp_addr           = get_addr(scan_counters, tx3_amp_base_addr, tx3_amp_order, nb_elements_per_counter[tx3_amp_order]);
                        
                        //    
                        uint32_t  tx4_freq_base_addr     = ((0x3FF << 0 ) & *ram_adr_c4_ptr) >> 0;
                        uint32_t  tx4_freq_order         = ((0xF   << 12) & *ram_orders_ptr) >> 12;
                        uint32_t  tx4_freq_addr          = get_addr(scan_counters, tx4_freq_base_addr, tx4_freq_order, nb_elements_per_counter[tx4_freq_order]);
                        //phase
                        uint32_t  tx4_phase_base_addr    = ((0x3FF << 0) & *ram_adr_c4b_ptr) >> 0;
                        uint32_t  tx4_phase_order        = ((0xF   << 16) & *ram_orders_ptr) >> 16;
                        uint32_t  tx4_phase_addr         = get_addr(scan_counters, tx4_phase_base_addr, tx4_phase_order, nb_elements_per_counter[tx4_phase_order]);
                        //amp
                        uint32_t  tx4_amp_base_addr      = ((0x3FF << 11) & *ram_adr_c4b_ptr) >> 11;
                        uint32_t  tx4_amp_order          = ((0xF   << 22) & *ram_adr_c4b_ptr) >> 22;
                        uint32_t  tx4_amp_addr           = get_addr(scan_counters, tx4_amp_base_addr, tx4_amp_order, nb_elements_per_counter[tx4_amp_order]);


                        //save the event
                        //save the timer
                        *events_base_ptr = ram_timer_ptr[timer_addr];
                        events_base_ptr++;
                        //the ttl
                        *events_base_ptr = 0xFFFF & *ram_ttl_ptr;
                        events_base_ptr++;
                        //TX1
                        //freq
                        *events_base_ptr = ram_freq1_ptr[tx1_freq_addr];
                        events_base_ptr++;
                        //phase + amplitude
                        *events_base_ptr = (ram_amp1_ptr[tx1_amp_addr]&0xffff)<<16 | (ram_phase1_ptr[tx1_phase_addr]&0xffff);
                        events_base_ptr++;
                        //TX2
                        *events_base_ptr = ram_freq2_ptr[tx2_freq_addr];
                        events_base_ptr++;
                        *events_base_ptr = (ram_amp2_ptr[tx2_amp_addr]&0xffff)<<16 | (ram_phase2_ptr[tx2_phase_addr]&0xffff);
                        events_base_ptr++;
                        //TX3
                        *events_base_ptr = ram_freq3_ptr[tx3_freq_addr];
                        events_base_ptr++;
                        *events_base_ptr = (ram_amp3_ptr[tx3_amp_addr]&0xffff)<<16 | (ram_phase3_ptr[tx3_phase_addr]&0xffff);
                        events_base_ptr++;
                        //TX4
                        *events_base_ptr = ram_freq4_ptr[tx4_freq_addr];
                        events_base_ptr++;
                        *events_base_ptr = (ram_amp4_ptr[tx4_amp_addr]&0xffff)<<16 | (ram_phase4_ptr[tx4_phase_addr]&0xffff);
                        events_base_ptr++;

                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;

                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr++;
                        //next place is byte 128 (80h)
                        ///events_base_ptr += 29;
                        
                        /*
                        *events_base_ptr = event_buffer[0];
                        *events_base_ptr = nb_of_all_events;
                        events_base_ptr += 1;
                        *events_base_ptr = 0;
                        events_base_ptr += 1;
                        */
                        //printf("event %p : %lx \n", ram_func_ptr, *events_base_ptr);
                        //prepare to the next addr in the ocr





                        nb_of_events_treated++; 
                        nb_of_all_events++;

                        //printf("event treated : %d \n", nb_of_events_treated);

                        

                        //if the number of events treated is sufficient, send the 1st part of events.
                        if (nb_of_events_treated == 128) {
                            //nb_bytes_to_send = 128 * 128; //bytes per event * nb_event
                            //nb_dma_transfer = 1;
                                                        
                            //ongoing transfer needs to finish first
                            if (is_second_trans){
                                while (fpga_dma_read_bit(FPGA_DMA_vaddr_void, FPGA_DMA_STATUS, FPGA_DMA_DONE) == 0){
                                }

                                //reset the controls
                                fpga_dma_write_bit(FPGA_DMA_vaddr_void,
                                    FPGA_DMA_CONTROL,
                                    FPGA_DMA_GO,
                                    0);
                                fpga_dma_write_bit(FPGA_DMA_vaddr_void, //clean the done bit
                                    FPGA_DMA_STATUS,
                                    FPGA_DMA_DONE,
                                    0);
                            }

                            //go again
                            fpga_dma_write_reg(FPGA_DMA_vaddr_void,   //set source address
                                FPGA_DMA_READADDRESS,
                                (uint32_t)DMA_TRANSFER_1_SRC_DMAC);

                            fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
                                FPGA_DMA_LENGTH,
                                DMA_FULL_BURST_IN_BYTES);

                            fpga_dma_write_bit(FPGA_DMA_vaddr_void,//start transfer
                                FPGA_DMA_CONTROL,
                                FPGA_DMA_GO,
                                1);

                            //printf("1 sending  %d events \n", nb_of_events_treated);
                            is_second_trans = true;

                            
                        }


                        //if there are still more events, send using 2nd part
                        if (nb_of_events_treated == 256) {
                            //ongoing transfer needs to finish first
                            while (fpga_dma_read_bit(FPGA_DMA_vaddr_void, FPGA_DMA_STATUS, FPGA_DMA_DONE) == 0){
                            }
                            //reset the controls
                            fpga_dma_write_bit(FPGA_DMA_vaddr_void,
                                FPGA_DMA_CONTROL,
                                FPGA_DMA_GO,
                                0);
                            fpga_dma_write_bit(FPGA_DMA_vaddr_void, //clean the done bit
                                FPGA_DMA_STATUS,
                                FPGA_DMA_DONE,
                                0);

                            //go again
                            fpga_dma_write_reg(FPGA_DMA_vaddr_void,   //set source address
                                FPGA_DMA_READADDRESS,
                                (uint32_t)DMA_TRANSFER_2_SRC_DMAC);

                            fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
                                FPGA_DMA_LENGTH,
                                DMA_FULL_BURST_IN_BYTES);

                            fpga_dma_write_bit(FPGA_DMA_vaddr_void,//start transfer
                                FPGA_DMA_CONTROL,
                                FPGA_DMA_GO,
                                1);

                            //printf("2 sending  %d events\n", nb_of_events_treated);

                            //reset the nb_events_treated
                            nb_of_events_treated = 0;

                            //reset the pointer
                            events_base_ptr = (uint32_t*)events_base;
                            
                        }


                        

                        if ((*ram_func_ptr & 0xff) == 0x08) {
                            ram_func_ptr = base_rams + 0  * STEP_32b_RAM;
                            ram_ttl_ptr  = base_rams + 1  * STEP_32b_RAM;
                            break;
                        }
                        else {
                            ram_func_ptr++;
                            ram_ttl_ptr++;
                        }


                    }

                }
            }
        }
    }

    /*
    for (int i = 0; i < _4D; i++) {
        printf("counters %d at %x \n", i, scan_counters[i]);
    }


    printf("\n nb of events to transfer %d \n", nb_of_all_events);
    */

    // --------------clean up our memory mapping and exit -----------------//
    if (munmap(events_base, DDR_EVENTS_SPAN) != 0) {
        printf("ERROR: munmap() events_base failed...\n");
        close(fd);
        return(1);
    }

    if (munmap(reserved_mem_base, HPS_RESERVED_SPAN) != 0) {
        printf("ERROR: munmap()  reserved_mem_base failed...\n");
        close(fd);
        return(1);
    }

    close(fd);


    //return 0;
    return nb_of_all_events;

}