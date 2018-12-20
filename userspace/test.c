#include "std_includes.h"
#include "module_loader.h"
#include "shared_memory.h"
#include "log.h"
#include "config.h"

#define MAX_RXDATA_BYTES 4194304

//25,488 to read 4194304 bytes with mmap + mcopy

void test_read_rxdata_full(uint32_t size, int print_count) {
	char* rxdata_file = "/dev/rxdata";
	int fd = open(rxdata_file, O_RDONLY);
	if (fd < 0) {
		log_error_errno("Unable to open rxdata (%s)", rxdata_file);
		return;
	}

	int nbytes = size;
	int32_t* buffer = malloc(size);

	struct timespec tstart = { 0,0 }, tend = { 0,0 };

	clock_gettime(CLOCK_MONOTONIC, &tstart);
	read(fd, buffer, nbytes);
	clock_gettime(CLOCK_MONOTONIC, &tend);
	log_info("read (%d bytes): %.3f ms", nbytes,
		(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);

	for (int i = 0; i < print_count; i++) {
		printf("%d: %d\n", i, buffer[i]);
	}

	free(buffer);
	close(fd);
}


void test_read_rxdata_partial(int nwords, int print_count) {
	struct timespec tstart = { 0,0 }, tend = { 0,0 };

	clock_gettime(CLOCK_MONOTONIC, &tstart);

	char* rxdata_file = "/dev/rxdata";
	int fd = open(rxdata_file, O_RDONLY);
	if (fd < 0) {
		log_error_errno("Unable to open rxdata (%s)", rxdata_file);
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &tend);
	log_info("open rxdata: %.3f ms", 
		(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);


	int nbytes = nwords * sizeof(int32_t);
	int32_t* buffer = malloc(nbytes);


	while (true) {
		printf("type enter to read %d words\n", nwords);
		fflush(stdout);
		getchar();

		clock_gettime(CLOCK_MONOTONIC, &tstart);
		if (lseek(fd, 0, SEEK_SET) < 0) {
			log_error_errno("unable to lseek to 0");
			close(fd);
			return;
		}

		int nread = read(fd, buffer, nbytes);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		log_info("read+seek (%d bytes): %.3f ms", nread,
			(tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000.0f);

		for (int i = 0; i < print_count; i++) {
			printf("%d: %d\n", i, buffer[i]);
		}
	}

	free(buffer);
	close(fd);
}


void test_read_lockdata_partial(int nwords, int print_count) {
	char* lockdata_file = "/dev/lockdata";
	int fd = open(lockdata_file, O_RDONLY);
	if (fd < 0) {
		log_error_errno("Unable to open lockdata (%s)", lockdata_file);
		return;
	}

	int nbytes = nwords * sizeof(int64_t);
	int64_t* buffer = malloc(nbytes);

	while (true) {
		printf("type enter to read %d words\n", nwords);
		fflush(stdout);
		getchar();

		if (lseek(fd, 0, SEEK_SET) < 0) {
			log_error_errno("unable to lseek to 0");
			close(fd);
			return;
		}

		read(fd, buffer, nbytes);
		for (int i = 0; i < print_count; i++) {
			printf("%x %llx\n", i, buffer[i]);
		}
	}

	free(buffer);
	close(fd);
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

	/*
	printf("type enter to read full data\n");
	fflush(stdout);
	getchar();
	test_read_rxdata_full(MAX_RXDATA_BYTES, 100);
	*/

	//test_read_rxdata_partial(1024*1024/4, 10); //1Mo

	test_read_lockdata_partial(10, 1);

	return 0;
}