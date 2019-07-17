#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>

#define BASE_MEM 0xC8000000

int32_t* map_csr_base_ptr() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);

	//EPCQ_CONTROLLER = 0xff220040, but for some reason we cannot mmap it directly
	int span = 2000;
	int32_t* mapped_ptr = (int32_t*)mmap(NULL, span, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0xff220000);
	if (mapped_ptr == MAP_FAILED) {
		perror("mmap csr failed");
		exit(1);
	}
	
	close(fd);
	return mapped_ptr + 0x40;
}

int32_t* map_mem_base_ptr() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	int32_t* mapped_ptr = mmap(NULL, 0x3ffffff, PROT_WRITE, MAP_SHARED, fd, BASE_MEM);
	if (mapped_ptr == MAP_FAILED) {
		perror("mmap mem failed");
		exit(1);
	}

	close(fd);
	return mapped_ptr;
}


void wr(int* csr_base_ptr, int cmd_reg, int value){
    int* csr_ptr=csr_base_ptr+cmd_reg;
    *csr_ptr= value;
}

int rd(int* csr_base_ptr, int cmd_reg){
    int* csr_ptr=csr_base_ptr+cmd_reg;
    return *csr_ptr;
}


int rd_flash(int* csr_base_ptr, int* mem_base_ptr, int addr){
    wr(csr_base_ptr,0x4,0x00000000); //use 1 line
    wr(csr_base_ptr,0,0x101);
    wr(csr_base_ptr,5,0x03);
    return rd(mem_base_ptr,addr);
}

void write_enable(int* csr_base_ptr){
	wr(csr_base_ptr,0x7,0x00000006);
	wr(csr_base_ptr,0x8,0x1);
}
void erase_sector(int* csr_base_ptr){
    wr(csr_base_ptr,7,0x004D8);
    wr(csr_base_ptr,9,0x00000);
    wr(csr_base_ptr,0xA,1);
    wr(csr_base_ptr,8,1);
}
void erase_die(int* csr_base_ptr){
    write_enable(csr_base_ptr);
    wr(csr_base_ptr,7,0x004C4);
    wr(csr_base_ptr,9,0x00000);
    wr(csr_base_ptr,0xA,1);
    wr(csr_base_ptr,8,1);
}


void write_memory(int* csr_base_ptr, int* mem_base_ptr, int addr, int value){
	wr(csr_base_ptr,0x4,0x00000000); //use 1 line
	wr(csr_base_ptr,0x0,0x00000101);
	wr(csr_base_ptr,8,1);
	wr(csr_base_ptr,0x6,0x00007002);
	wr(mem_base_ptr,addr,value);
}

void rd_memory(int* csr_base_ptr, int* mem_base_ptr, int start_addr, int nb_of_bytes, int rd_data[]){
    int* new_mem_base= mem_base_ptr+start_addr;
    wr(csr_base_ptr,0x4,0x00000000); //use 1 line
    wr(csr_base_ptr,0,0x101);
    wr(csr_base_ptr,5,0x03);
    for(int i=0; i< (nb_of_bytes/4); i++){
        rd_data[i] = rd(new_mem_base, i);
    }
}


void write_memory_block(int* csr_base_ptr, int* mem_base_ptr, int nb_of_bytes, int wr_data[]){
	wr(csr_base_ptr,0x4,0x00000000); //use 1 line
	wr(csr_base_ptr,0x0,0x00000101);
	wr(csr_base_ptr,8,1);
	wr(csr_base_ptr,0x6,0x00007002);
	for(int i=0; i< (nb_of_bytes/4)-1; i++){
        wr(mem_base_ptr,i,wr_data[i]);
        printf("written address %d / %d  : %x \n",i, (nb_of_bytes/4),wr_data[i]); 
    }
}


void fastwrite_memory_block(int* csr_base_ptr, int* mem_base_ptr, int nb_of_bytes, int wr_data[]){
	write_enable(csr_base_ptr);
	wr(csr_base_ptr,0x1,0x00000001); 
	wr(csr_base_ptr,0x4,0x00000220); 
	wr(csr_base_ptr,0x0,0x00000101);
	wr(csr_base_ptr,0x6,0x00007012);
	for(int i=0; i<= (nb_of_bytes/4)-1; i++){
        wr(mem_base_ptr,i,wr_data[i]);
        if(i%511==0)printf("done %d / %d \n",i, nb_of_bytes>>2 );
    }
}


void nvcr_set(int* csr_base_ptr){
    write_enable(csr_base_ptr);
    wr(csr_base_ptr,0x7,0x000002B1);
    wr(csr_base_ptr,0xA,0x0000FFFE);
    wr(csr_base_ptr,8,1);
}


int epcq_image_main(int argc, char** argv) {
	int* csr_base_ptr = map_csr_base_ptr();
	int* mem_base_ptr = map_mem_base_ptr();
    int rpd_size;

     //open file rpd
     FILE* frd=fopen("output_file_auto.rpd","r");
     //check size rpd
     fseek(frd, 0L, SEEK_END);
     rpd_size=ftell(frd);
     printf("file size = %d \n", rpd_size); 
     rewind(frd); 
        
     int* wr_value_array=malloc(rpd_size); 
     nvcr_set(csr_base_ptr);
     write_enable(csr_base_ptr);
     printf("Start ERASING FLASH DIE : wait\n"); 
     erase_die(csr_base_ptr); //bloque le HPS pendant 1mn...
        
     fread(wr_value_array,sizeof(char),rpd_size,frd); 
     fastwrite_memory_block(csr_base_ptr,mem_base_ptr,rpd_size,wr_value_array);
     
	 return 0;
}
    