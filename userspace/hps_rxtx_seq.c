#include "hps_rxtx_seq.h"
#include "hps_sequence.h"
#include "log.h"

static pthread_t thread_rxtx;
//static pthread_t thread_grad;
static pthread_attr_t attr;


static void* rxtx_seq_thread(void* arg) {
	printf("events sent : %d\n",create_events());
	pthread_exit(0);
}

/*
static void* grad_seq_thread(void* arg) {
	for (int i = 0; i <= 512; i++) {
		printf("events grad : %d\n", i);
		usleep(1000);
	}
	pthread_exit(0);
}
*/

bool start_rxtx_seq(void) {
	printf("here");
	//start
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if (pthread_create(&thread_rxtx, &attr, rxtx_seq_thread, NULL) != 0) {
		log_error("Unable to create rxtx thread!");
		return false;
	}
	
	/*
	if (pthread_create(&thread_grad, &attr, grad_seq_thread, NULL) != 0) {
		log_error("Unable to create grad thread!");
		return false;
	}

	
	if (pthread_join(thread_rxtx,NULL) != 0) {
		log_error("Unable to join rxtx thread!");
		return false;
	}
	if (pthread_join(thread_grad, NULL) != 0) {
		log_error("Unable to join grad thread!");
		return false;
	}
	*/

	//pthread_exit(NULL);
	return true;
}

bool stop_rxtx_seq(void) {
	printf("cancelling thread \n");
	if (pthread_cancel(thread_rxtx) != 0) {
		log_error("Unable to cancel rxtx_seq_thread");
		return false;
	}
	return true;
}