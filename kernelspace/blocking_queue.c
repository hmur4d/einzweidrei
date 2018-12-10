#include "blocking_queue.h"
#include "klog.h"

static DEFINE_KFIFO(fifo, int8_t, FIFO_SIZE);
static DEFINE_KFIFO(fifo_timestamps, u64, FIFO_SIZE);
static DECLARE_WAIT_QUEUE_HEAD(waitqueue);

static u64 time_get_ns(void) {
	return ktime_to_ns(ktime_get());
}

void blocking_queue_reset(void) {
	kfifo_reset(&fifo);
	kfifo_reset(&fifo_timestamps);
}

bool blocking_queue_is_empty(void) {
	return kfifo_is_empty(&fifo);
}

bool blocking_queue_add(uint8_t value) {
	u64 now = time_get_ns();
	if (!kfifo_put(&fifo, value) || !kfifo_put(&fifo_timestamps, now)) {
		klog_warning("Unable to add item to blocking queue\n");
		klog_info("queue size: %d / %d\n", kfifo_len(&fifo), kfifo_size(&fifo));
		return false;
	}

	klog_info("added item to blocking queue, now = %llu\n", now);
	//klog_info("queue size: %d / %d\n", kfifo_len(&fifo), kfifo_size(&fifo));

	wake_up_interruptible(&waitqueue);
	return true;
}

bool blocking_queue_take(uint8_t* value) {
	if (kfifo_is_empty(&fifo)) {
		//let's wait until there's something in the fifo
		if (wait_event_interruptible(waitqueue, !kfifo_is_empty(&fifo))) {
			return false;
		}
	}

	u64 now = time_get_ns();
	klog_info("taking item from blocking queue, now=%llu\n", now);

	u64 timestamp = 0;
	if (!kfifo_get(&fifo, value) || !kfifo_get(&fifo_timestamps, &timestamp)) {
		klog_error("kfifo_get failed!\n");
		return false;
	}

	long diff = now - timestamp;
	klog_info("time elapsed between add and take: %ld ns\n", diff);

	//klog_info("queue size: %d / %d\n", kfifo_len(&fifo), kfifo_size(&fifo));
	return true;
}