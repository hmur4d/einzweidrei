#ifndef _UDP_H_
#define _UDP_H_

#include "std_includes.h"


typedef struct  {
	int type;
	int feature;
	int cmdPort;
	int dataPort;
}device_info_t;

typedef struct  {
	int cmd;
	device_info_t cameleon;
	device_info_t sequence;
	device_info_t gradient;
	device_info_t lock;
	device_info_t shim;
}udp_info_t;


typedef struct {
	int     fd;
	struct sockaddr_in addr;
} conn_udp_t;

#define UDP_SLEEP_TIME 2000000

#endif

bool udp_broadcaster_start();

bool udp_broadcaster_stop();
