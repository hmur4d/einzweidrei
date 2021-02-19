#include "hps_sequence.h"
#include "fpga_dmac_api.h"

#define DMA_TRANSFER_WORDS 	31 //in 64bits word
#define DMA_TRANSFER_SIZE 	DMA_TRANSFER_WORDS*8 //in bvtes

#define LW_BASE 0xff200000
#define LW_SPAN  1048576
#define FPGA_DMAC_QSYS_ADDRESS 0x00020080
#define FPGA_DMAC_ADDRESS ((uint8_t*)LW_BASE+FPGA_DMAC_QSYS_ADDRESS)

#define HPS_OCR_ADDRESS			0x20000000 
#define HPS_OCR_SPAN			0x20000000     //span in bytes


//#define HPS_OCR_ADDRESS 0xFFE00000


//fifo is connected directly to dma
#define DMA_TRANSFER_DST_DMAC 0x0

#define DMA_FULL_BURST_IN_BYTES  16384 //1024*16 this is 128 event

#define DMA_TRANSFER_1_SRC_DMAC (HPS_OCR_ADDRESS)
#define DMA_TRANSFER_2_SRC_DMAC (HPS_OCR_ADDRESS+DMA_FULL_BURST_IN_BYTES)

#define _PS 0
#define _DS 1
#define _1D 2
#define _2D 3
#define _3D 4
#define _4D 5

uint32_t scan_counters[6] = { 0,0,0,0,0,0 };
uint32_t loopa_counters[2] = { 0,0 };

// nb of elements per counter to be used for the modulus
uint32_t nb_elements_per_counter[16] = { 0,
                                      0,0,0,0,
                                      0,0,0,0,0,
                                      0,0,0,0,0
};


uint32_t static get_addr(uint32_t* scan_counter, uint32_t base_address, uint32_t order, uint32_t nb_of_values) {
    return (scan_counter[order] + base_address);//%nb_of_values;
}

uint32_t create_events(void) {
    int fd;
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return(1);
    }
    
    //access onchip hps
    void* ocr_base;
    //mmap from HPS OCR to end HPS OCR
    ocr_base = mmap(NULL, HPS_OCR_SPAN, (PROT_READ | PROT_WRITE),
        MAP_SHARED, fd, HPS_OCR_ADDRESS);

    if (ocr_base == MAP_FAILED) {
        printf("ERROR: mmap() ocr_base failed...\n");
        close(fd);
        return(1);
    }
    uint32_t* ocr_base_ptr = (uint32_t*)ocr_base;
    

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


    //uint32_t nb_bytes_to_send = 0;
   //uint32_t nb_dma_transfer = 0;
    //uint32_t nb_remainder = 0;


    //RAMS
    uint32_t* base_rams = (uint32_t*)reserved_mem_base;
    uint32_t offset_ram_func = 0 * STEP_32b_RAM;
    uint32_t offset_ram_ttl = 1 * STEP_32b_RAM;
    uint32_t offset_ram_timer = 49 * STEP_32b_RAM;

    uint32_t* current_event_ptr = base_rams + offset_ram_func;
    uint32_t* ram_timer_ptr = base_rams + offset_ram_timer;
    uint32_t* ram_ttl_ptr = base_rams + offset_ram_ttl;

    uint32_t* base_of_regs = (uint32_t*)reserved_mem_base + (131072 * 87);
    uint32_t  nb_dimensions[6];
    nb_dimensions[_DS] = *(base_of_regs + 7);
    nb_dimensions[_1D] = *(base_of_regs + 8);
    nb_dimensions[_2D] = *(base_of_regs + 9);
    nb_dimensions[_3D] = *(base_of_regs + 10);
    nb_dimensions[_4D] = *(base_of_regs + 11);
    nb_dimensions[_PS] = *(base_of_regs + 93);

    uint32_t nb_of_all_events = 0;
    uint32_t event_buffer[2] = { 0,0 };
    
    /*
    for (int i = 0; i < 5; i++) {
        printf("ram ttl %d at %x \n", i, ram_ttl_ptr[i]);
    }
    */
    for (int i = 0; i < _4D; i++) {
        printf("NB scans %d at %x \n", i, nb_dimensions[i]);
    }


    uint32_t nb_of_events_treated = 0; 

    bool is_second_trans = false;

    for (scan_counters[_4D] = 0; scan_counters[_4D] <= nb_dimensions[_4D]; scan_counters[_4D]++) {
        for (scan_counters[_3D] = 0; scan_counters[_3D] <= nb_dimensions[_3D]; scan_counters[_3D]++) {
            for (scan_counters[_2D] = 0; scan_counters[_2D] <= nb_dimensions[_2D]; scan_counters[_2D]++) {
                //printf("\n\n");
                for (scan_counters[_1D] = 0; scan_counters[_1D] <= nb_dimensions[_1D]; scan_counters[_1D]++) {
                    while (true) {

                        //get the current event
                        uint32_t current_event = *current_event_ptr;
                        //printf( "%x \n", current_event);

                        //look at the timer addr and timer order
                        uint32_t timer_base_addr = ((0x3FF << 22) & current_event) >> 22;
                        uint32_t timer_order = ((0xF << 18) & current_event) >> 18;

                        uint32_t timer_addr = get_addr(scan_counters, timer_base_addr, timer_order, 10);
                        //printf("timer_base_addr: %d , timer_order: %d, timer_addr: %d \n", timer_base_addr, timer_order, timer_addr);

                        //buffer the event
                        event_buffer[0] = (0xFFFFFFFF & ram_timer_ptr[timer_addr]) << 0; //timer


                        //look at the ttl
                        uint16_t rxtx_ttl = 0xFFFF & *ram_ttl_ptr;
                        event_buffer[1] = rxtx_ttl;
                        //printf(" %x ", event_buffer[1]);


                        //save the event in the onchip 
                        
                        *ocr_base_ptr = event_buffer[0];
                        ocr_base_ptr++;
                        *ocr_base_ptr = event_buffer[1];
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;

                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr++;
                        //next place is byte 128 (80h)
                        ///ocr_base_ptr += 29;
                        
                        /*
                        *ocr_base_ptr = event_buffer[0];
                        *ocr_base_ptr = nb_of_all_events;
                        ocr_base_ptr += 1;
                        *ocr_base_ptr = 0;
                        ocr_base_ptr += 1;
                        */
                        //printf("event %p : %lx \n", current_event_ptr, *ocr_base_ptr);
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

                            printf("1 sending  %d events \n", nb_of_events_treated);
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

                            printf("2 sending  %d events\n", nb_of_events_treated);

                            //reset the nb_events_treated
                            nb_of_events_treated = 0;

                            //reset the pointer
                            ocr_base_ptr = (uint32_t*)ocr_base;
                            
                        }


                        

                        if ((current_event & 0xff) == 0x08) {
                            current_event_ptr = base_rams + offset_ram_func;
                            ram_ttl_ptr = base_rams + offset_ram_ttl;
                            break;
                        }
                        else {
                            current_event_ptr++;
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
    if (munmap(ocr_base, HPS_OCR_SPAN) != 0) {
        printf("ERROR: munmap() ocr_base failed...\n");
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