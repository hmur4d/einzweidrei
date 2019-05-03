#include "udp_broadcaster.h"
#include "log.h"
#include "commands.h"
#include "config.h"
#include "pthread.h"
#include "clientgroup.h"

conn_udp_t connUDP;
udp_info_t udpInfo;

static bool initialized = false;
static pthread_mutex_t mutex;
static pthread_t thread;


static void init_device_info(udp_info_t * pUDPinfo) {
	pUDPinfo->cmd = CMD_DEVICE_INFO;

	pUDPinfo->cameleon= (device_info_t) {
		.type = DEVICE_TYPE_CAMELEON4,
		.feature = DEVICE_FEATURE_CAMELEON,
		.cmdPort = COMMAND_PORT,
		.dataPort = MONITORING_PORT,
	};
	pUDPinfo->sequence = (device_info_t) {
		.type = DEVICE_TYPE_CAMELEON4,
		.feature = DEVICE_FEATURE_SEQUENCER,
		.cmdPort = COMMAND_PORT,
		.dataPort = SEQUENCER_PORT,
	};
	pUDPinfo->gradient = (device_info_t) {
		.type = DEVICE_TYPE_CAMELEON4,
		.feature = DEVICE_FEATURE_GRADIENT,
		.cmdPort = COMMAND_PORT,
		.dataPort = -1,
	};
	pUDPinfo->lock = (device_info_t) {
		.type = DEVICE_TYPE_CAMELEON4,
			.feature = DEVICE_FEATURE_LOCK,
			.cmdPort = COMMAND_PORT,
		.dataPort = LOCK_PORT,
	};

	return;
}


static void init_udp_broadcaster() {

	init_device_info(&udpInfo);


	connUDP.fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (connUDP.fd < 0) {
		log_error("Failed to create UDP socket file descriptor");
	}
	else {

		int broadcastEnable = 1;
		int ret = setsockopt(connUDP.fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

		if (ret != 0) {
			log_error_errno("UDP socket set option error");
		}

		connUDP.addr.sin_family = AF_INET;
		connUDP.addr.sin_addr.s_addr = (unsigned long)0xFFFFFFFF;
		connUDP.addr.sin_port = htons(UDP_PORT);


		//s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
		log_info("UDP socket opened on %d", inet_ntoa(connUDP.addr.sin_addr));

	}
}

static int send_udp_info() {
	log_debug("send_udp_info fd=%d, addr=%s, port=%d, feature=%d", connUDP.fd, inet_ntoa(connUDP.addr.sin_addr), connUDP.addr.sin_port,udpInfo.cameleon.feature);

	return sendto(connUDP.fd, &udpInfo, sizeof(udp_info_t), 0, (struct sockaddr*)&(connUDP.addr), sizeof(connUDP.addr));
}

static void* udp_broadcaster_thread(void* data) {
	log_info("UDP thread started");
	while (true) {
		
		usleep(UDP_SLEEP_TIME);
		//send udp info if not connected
		
		if (!clientgroup_is_connected()) {
			log_debug("UDP client not is connected, send info");
			pthread_mutex_lock(&mutex);
			int res=send_udp_info();
			if (res < 0) {
				log_error_errno("UDP send result %d", res);
			}
			pthread_mutex_unlock(&mutex);
		}
		else {
			log_debug("UDP client connected do not send info");
		}
	}
	return NULL;
}

bool udp_broadcaster_start() {

	log_debug("Creating udp mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	log_debug("init udp");
	init_udp_broadcaster();
	log_debug("Creating udp thread");
	if (pthread_create(&thread, NULL, udp_broadcaster_thread, NULL) != 0) {
		log_error("Unable to create udp thread!");
		return false;
	}

	initialized = true;
	return true;
}

bool udp_broadcaster_stop() {
	if (!initialized) {
		log_warning("Trying to stop udp, but it isn't initialized!");
		return true;
	}

	if (pthread_cancel(thread) != 0) {
		log_error("Unable to cancel udp thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("Unable to join udp thread");
		return false;
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	return true;
}