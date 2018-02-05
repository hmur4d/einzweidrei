#include "blocking_queue.h"
#include "klog.h"

static long time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

void blocking_queue_reset(blocking_queue_t* queue) {
	kfifo_reset(&queue->fifo);
}

bool blocking_queue_is_empty(blocking_queue_t* queue) {
	return kfifo_is_empty(&queue->fifo);
}

bool blocking_queue_add(blocking_queue_t* queue, int8_t value) {
	if (kfifo_in(&queue->fifo, &value, 1) != 1) {
		klog_warning("Unable to add item to blocking queue, fifo full?\n");
		return false;
	}

	queue->last_push = time_get_ns();
	wake_up_interruptible(&queue->waitqueue);
	return true;
}

bool blocking_queue_take(blocking_queue_t* queue, int8_t* value) {
	if (kfifo_is_empty(&queue->fifo)) {
		//let's wait until there's something in the fifo
		if (wait_event_interruptible(queue->waitqueue, !kfifo_is_empty(&queue->fifo))) {
			return false;
		}
	}

	int diff = time_get_ns() - queue->last_push;
	klog_info("time elapsed between push and get: %d ns\n", diff);

	size_t nbytes = 1;
	if (kfifo_out(&queue->fifo, value, nbytes) != nbytes) {
		return false;
	}

	return true;
}