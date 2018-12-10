#include "blocking_queue.h"
#include "klog.h"

#define NO_VALUE 255

//use a ring buffer
int size = 0;
int next_read = 0;
int next_write = 0;
int8_t values[FIFO_SIZE];
ktime_t timestamps[FIFO_SIZE];

static DECLARE_WAIT_QUEUE_HEAD(waitqueue);

void blocking_queue_reset(void) {
	size = 0;
	next_read = 0;
	next_write = 0;
}

bool blocking_queue_is_empty(void) {
	return size == 0;
}

bool blocking_queue_add(uint8_t value) {
	if (size + 1 == FIFO_SIZE) {
		klog_warning("Unable to add item to blocking queue, size=%d\n", size);
		return false;
	}

	timestamps[next_write] = ktime_get();
	values[next_write++] = value;
	size++;
	if (next_write >= FIFO_SIZE) {
		next_write = 0;
	}


	klog_info("added item to blocking queue, size=%d / %d\n", size, FIFO_SIZE);
	wake_up_interruptible(&waitqueue);
	return true;
}

bool blocking_queue_take(uint8_t* value) {
	if (size == 0) {
		//let's wait until there's something in the fifo
		if (wait_event_interruptible(waitqueue, size != 0)) {
			return false;
		}
	}

	ktime_t timestamp = timestamps[next_read];
	*value = values[next_read++];
	size--;
	if (next_read >= FIFO_SIZE)
		next_read = 0;
	
	ktime_t now = ktime_get();
	ktime_t delta = ktime_sub(now, timestamp);
	u64 delta_ns = ktime_to_ns(delta);
	klog_info("took item to blocking queue, size=%d / %d, took %llu ns\n", size, FIFO_SIZE, delta_ns);

	return true;
}