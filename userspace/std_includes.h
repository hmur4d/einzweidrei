#ifndef _STD_INCLUDES_H_
#define _STD_INCLUDES_H_

/*
Common system includes, used in several files.
Project includes should not be added here, only unmodifiable system includes.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <ifaddrs.h>

#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <netinet/in.h> 

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/spi/spidev.h>

#endif