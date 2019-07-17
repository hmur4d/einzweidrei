#include "std_includes.h"

#define BASE_MEM 0xC8000000

static void fail(char* message) {
	perror(message);
	exit(1);
}

static void show_progress_bar(int value, int max) {
	static int next_indicator = 0;
	int steps = 50;
	int progress = (int)((value / (double)max) * steps);
	char bar[steps+1];
	char *indicators = "|/-\\";

	for (int i = 0; i <= progress; i++) {
		bar[i] = '=';
	}

	if(progress < steps) {
		bar[progress] = indicators[next_indicator];
		next_indicator = (next_indicator + 1) % 4;

		for (int i = progress + 1; i < steps; i++) {
			bar[i] = ' ';
		}
	}
	bar[steps] = '\0';

	printf("\r[%s] %d / %d", bar, value, max);
	fflush(stdout);
}

static int32_t* read_image_file(char* filename, int* nbytes) {
	printf("image file: %s\n", filename);
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		fail("unable to open image file");
	}

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	printf("image size: %d\n", size);

	void* data = malloc(size);
	if (data == NULL) {
		fail("malloc failed");
	}

	ssize_t total_read = 0;
	while (total_read < size) {
		ssize_t nread = fread(data + total_read, sizeof(char), size - total_read, f);
		if (nread < 0) {
			fail("Error reading file");
		}

		total_read += nread;
	}
	fclose(f);

	*nbytes = total_read;
	return (int32_t*)data;
}

static int32_t* map_csr_base_ptr() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);

	//EPCQ_CONTROLLER = 0xff220040, but for some reason we cannot mmap it directly
	int span = 2000;
	int32_t* mapped_ptr = (int32_t*)mmap(NULL, span, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0xff220000);
	if (mapped_ptr == MAP_FAILED) {
		fail("mmap csr failed");
	}
	
	close(fd);
	return mapped_ptr + 0x40;
}

static int32_t* map_mem_base_ptr() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	int32_t* mapped_ptr = mmap(NULL, 0x3ffffff, PROT_WRITE, MAP_SHARED, fd, BASE_MEM);
	if (mapped_ptr == MAP_FAILED) {
		fail("mmap mem failed");
	}

	close(fd);
	return mapped_ptr;
}

static void wr(int32_t* csr_base_ptr, int cmd_reg, int32_t value) {
    *(csr_base_ptr + cmd_reg) = value;
}

static void write_enable(int32_t* csr_base_ptr) {
	wr(csr_base_ptr, 0x7, 0x00000006);
	wr(csr_base_ptr, 0x8, 0x1);
}

static void nvcr_set(int32_t* csr_base_ptr) {
	write_enable(csr_base_ptr);
	wr(csr_base_ptr, 0x7, 0x000002B1);
	wr(csr_base_ptr, 0xA, 0x0000FFFE);
	wr(csr_base_ptr, 8, 1);
}

static void erase_die(int32_t* csr_base_ptr) {
	printf("Erasing flash die: please wait...\n");
    write_enable(csr_base_ptr);
    wr(csr_base_ptr, 7, 0x004C4);
    wr(csr_base_ptr, 9, 0x00000);
    wr(csr_base_ptr, 0xA, 1);
    wr(csr_base_ptr, 8, 1);

	long utime = 110 * 1000 * 1000; //1m50
	long sleep_time = 300 * 1000; //300ms
	for (int i = 0; i < utime; i += sleep_time) {
		show_progress_bar(i, utime);
		usleep(sleep_time);
	}
	show_progress_bar(utime, utime);

	printf("\nErasing flash die: done.\n");
}

static void fastwrite_memory_block(int32_t* csr_base_ptr, int32_t* mem_base_ptr, int32_t* data, int nbytes) {
	printf("Writing image: please wait...\n");

	int nb_int32 = nbytes / sizeof(int32_t);
	write_enable(csr_base_ptr);
	wr(csr_base_ptr, 0x1, 0x00000001); 
	wr(csr_base_ptr, 0x4, 0x00000220); 
	wr(csr_base_ptr, 0x0, 0x00000101);
	wr(csr_base_ptr, 0x6, 0x00007012);

	for(int i=0; i<nb_int32; i++) {
        wr(mem_base_ptr, i, data[i]);

		if (i % 1024 == 0) {
			show_progress_bar(i, nb_int32);
		}
    }
	show_progress_bar(nb_int32, nb_int32);
	
	printf("\nWriting image: done.\n");
}

int epcq_image_main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: cameleon image <file.rpd>\n");
		exit(1);
	}

	char * filename = argv[1];
	int size = 0;
	int32_t* data = read_image_file(filename, &size);

	int32_t* csr_base_ptr = map_csr_base_ptr();
	nvcr_set(csr_base_ptr);
    erase_die(csr_base_ptr); 

	int32_t* mem_base_ptr = map_mem_base_ptr();
    fastwrite_memory_block(csr_base_ptr, mem_base_ptr, data, size);
     
	free(data);
	return 0;
}
    