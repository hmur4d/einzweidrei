#include "fpga_dma.h"
#include "fpga_dmac_api.h"

#define DMA_TRANSFER_WORDS 	31 //in 64bits word
#define DMA_TRANSFER_SIZE 	DMA_TRANSFER_WORDS*8 //in bvtes

#define LW_BASE 0xff200000
#define LW_SPAN  1048576
#define FPGA_DMAC_QSYS_ADDRESS 0x00020080
#define FPGA_DMAC_ADDRESS ((uint8_t*)LW_BASE+FPGA_DMAC_QSYS_ADDRESS)


#define HPS_OCR_ADDRESS 0xFFE00000
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
        FPGA_DMA_DOUBLEWORD_TRANSFERS |
        FPGA_DMA_END_WHEN_LENGHT_ZERO
    );
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,   //set source address
        FPGA_DMA_READADDRESS,
        (uint32_t)DMA_TRANSFER_SRC_DMAC);
    fpga_dma_write_reg(FPGA_DMA_vaddr_void,  //set destiny address
        FPGA_DMA_WRITEADDRESS,
        (uint32_t)DMA_TRANSFER_DST_DMAC);
    fpga_dma_write_reg(FPGA_DMA_vaddr_void, //set transfer size
        FPGA_DMA_LENGTH,
        DMA_TRANSFER_SIZE);
    fpga_dma_write_bit(FPGA_DMA_vaddr_void, //clean the done bit
        FPGA_DMA_STATUS,
        FPGA_DMA_DONE,
        0);

    //printf("Start DMA Transfer\n");
    fpga_dma_write_bit(FPGA_DMA_vaddr_void,//start transfer
        FPGA_DMA_CONTROL,
        FPGA_DMA_GO,
        1);
    printf("DMA Transfer Started\n");
    while (fpga_dma_read_bit(FPGA_DMA_vaddr_void, FPGA_DMA_STATUS,
        FPGA_DMA_DONE) == 0) {
    }
    printf("DMA Transfer Finished\n");


    // --------------clean up our memory mapping and exit -----------------//
    if (munmap(lw_vaddr, LW_SPAN) != 0) {
        printf("ERROR: munmap() failed...\n");
        close(fd);
        return(1);
    }

    close(fd);

    return(0);

}
