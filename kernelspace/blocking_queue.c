#include "blocking_queue.h"
#include "klog.h"

static DEFINE_KFIFO(fifo, int8_t, FIFO_SIZE);
static DECLARE_WAIT_QUEUE_HEAD(waitqueue);
static long last_push = 0;

static long time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

void blocking_queue_reset(void) {
	kfifo_reset(&fifo);
}

bool blocking_queue_is_empty(void) {
	return kfifo_is_empty(&fifo);
}

bool blocking_queue_add(int8_t value) {
	if (kfifo_in(&fifo, &value, 1) != 1) {
		klog_warning("Unable to add item to blocking queue, fifo full?\n");
		return false;
	}

	last_push = time_get_ns();
	wake_up_interruptible(&waitqueue);
	return true;
}

bool blocking_queue_take(int8_t* value) {
	if (kfifo_is_empty(&fifo)) {
		//let's wait until there's something in the fifo
		if (wait_event_interruptible(waitqueue, !kfifo_is_empty(&fifo))) {
			return false;
		}
	}

	int diff = time_get_ns() - last_push;
	klog_info("time elapsed between push and get: %d ns\n", diff);
	klog_info("queue size: %d\n", kfifo_size(&fifo));

	size_t nbytes = 1;
	if (kfifo_out(&fifo, value, nbytes) != nbytes) {
		return false;
	}

	return true;
}