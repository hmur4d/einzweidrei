#include "fpga_dma.h"
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
#define DMA_TRANSFER_SRC_DMAC HPS_OCR_ADDRESS

//fifo is connected directly to dma
#define DMA_TRANSFER_DST_DMAC 0x0

int transfer_to_fpga(uint32_t nb_of_events) {
    //open dev mem
    int fd;

    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
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

    //Do the transfer using the DMA in the FPGA
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,
        FPGA_DMA_CONTROL,
        FPGA_DMA_SOFTWARE_RESET
    );
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,
        FPGA_DMA_CONTROL,
        0
    );
    printf("REset DMA Controller\n");


    printf("Initializing DMA Controller\n");
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,
        FPGA_DMA_CONTROL,
        FPGA_DMA_QUADWORD_TRANSFERS |
        FPGA_DMA_END_WHEN_LENGHT_ZERO
    );
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,   //set source address
        FPGA_DMA_READADDRESS,
        (uint32_t)DMA_TRANSFER_SRC_DMAC);
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,  //set destiny address
        FPGA_DMA_WRITEADDRESS,
        (uint32_t)DMA_TRANSFER_DST_DMAC);


    uint32_t nb_bytes_to_send = 0;
    uint32_t nb_dma_transfer = 0;
    uint32_t nb_remainder = 0; 
    //the burst can only handle up to 1024 quadwords 18bytes so only 128 events
    if (nb_of_events <= 128) {
        nb_bytes_to_send = nb_of_events * 128;
        nb_dma_transfer = 1; 
    }
    else{
        nb_dma_transfer = nb_of_events / 128;
        nb_bytes_to_send = 128*128;
        nb_remainder = nb_of_events % 128;
    }

    //uint32_t nb_bytes_to_send = nb_of_events*128;
    //uint32_t nb_bytes_to_send = 1024 * 128;
    printf(" %d events so sending %d bytes %d times \n", nb_of_events, nb_bytes_to_send, nb_dma_transfer);

    //fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
    //    FPGA_DMA_LENGTH,
    //    nb_bytes_to_send);



    int i = 0; 
    while (1) {
        fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
            FPGA_DMA_LENGTH,
            nb_bytes_to_send);

        fpga_dma_write_bit(FPGA_DMA_vaddr_void,//start transfer
            FPGA_DMA_CONTROL,
            FPGA_DMA_GO,
            1);
        i++;
        
        while (fpga_dma_read_bit(FPGA_DMA_vaddr_void, FPGA_DMA_STATUS,
            FPGA_DMA_DONE) == 0) {
        }
        fpga_dma_write_bit(FPGA_DMA_vaddr_void,
            FPGA_DMA_CONTROL,
            FPGA_DMA_GO,
            0);
        fpga_dma_write_bit(FPGA_DMA_vaddr_void, //clean the done bit
            FPGA_DMA_STATUS,
            FPGA_DMA_DONE,
            0);
        printf("sending 128 blocks. %d : %d / %d \n",nb_remainder,i, nb_dma_transfer);

        if (i >= nb_dma_transfer) {
            if (nb_remainder == 0) break;
            else {
                printf("sending remainder \n");
                nb_bytes_to_send = 128 * nb_remainder;
                fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
                                    FPGA_DMA_LENGTH,
                                    nb_bytes_to_send);
                nb_remainder = 0;
                
            }
        }

    }
    // --------------clean up our memory mapping and exit -----------------//
    if (munmap(lw_vaddr, LW_SPAN) != 0) {
        printf("ERROR: munmap() failed...\n");
        close(fd);
        return(1);
    }

    close(fd);

    return(0);

}
