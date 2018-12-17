#include "std_includes.h"
#include "module_loader.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"

#define MAX_RXDATA_BYTES 4194304

//25,488 to read 4194304 bytes with mmap + mcopy

void test_mmap_devmem(uint32_t addr, uint32_t size, int print_count) {
	char* memory_file = "/dev/mem";
	int fd = open(memory_file, O_RDWR | O_SYNC);
	if (fd < 0) {
		log_error_errno("Unable to open shared memory (%s)", memory_file);
		return;
	}

	int nbytes = size;
	int32_t* buffer = malloc(size);

	int32_t* rxdata = (int32_t*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, addr);
	struct timespec tstart = { 0,0 }, tend = { 0,0 };

	clock_gettime(CLOCK_MONOTONIC, &tstart);
	memcpy(buffer, rxdata, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);
	log_info("memcpy (%d bytes): %.3f ms", nbytes,
		(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);

	for (int i = 0; i < print_count; i++) {
		printf("%d: %d\n", i, buffer[i]);
	}

	free(buffer);
	munmap(rxdata, size);
	close(fd);
}

uint32_t read_rxdata_addr() {
	int fd = open("/dev/rxdata", O_RDONLY);
	int nbytes = 4;
	uint8_t buffer[nbytes];
	if (read(fd, buffer, nbytes) != nbytes) {
		log_error_errno("Error while reading rxdata address");
		return 0;
	}

	uint32_t address = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
	return address;
}

int test_main(int argc, char ** argv) {
	int loglevel = get_log_level();
	if (!log_init(loglevel, LOG_FILE)) {
		return 1;
	}

	log_info("loading module...");
	if (!module_load(MODULE_PATH, "")) {
		log_error("Unable to load module (%s), exiting", MODULE_PATH);
		return 1;
	}

	log_info("reading rxdata address...");
	uint32_t addr = read_rxdata_addr();
	log_info("- address = 0x%x", addr);

	while(true) {
		printf("type enter to read data\n");
		fflush(stdout);
		getchar();
		test_mmap_devmem(addr, MAX_RXDATA_BYTES, 100);
	}
	

	return 0;
}