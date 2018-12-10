#include "blocking_queue.h"
#include "klog.h"

typedef struct {
	int8_t value;
	ktime_t time;
} timed_value_t;

//use a ring buffer
int size = 0;
int next_read = 0;
int next_write = 0;
timed_value_t values[QUEUE_CAPACITY];

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
	if (size + 1 == QUEUE_CAPACITY) {
		klog_warning("Unable to add item to blocking queue, size=%d\n", size);
		return false;
	}

	timed_value_t timedvalue = { .value = value,.time = ktime_get() };
	values[next_write++] = timedvalue;
	size++;
	if (next_write >= QUEUE_CAPACITY) {
		next_write = 0;
	}


	klog_info("added item to blocking queue, size=%d / %d\n", size, QUEUE_CAPACITY);
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

	timed_value_t timedvalue = values[next_read++];
	*value = timedvalue.value;
	size--;
	if (next_read >= QUEUE_CAPACITY)
		next_read = 0;
	
	ktime_t now = ktime_get();
	ktime_t delta = ktime_sub(now, timedvalue.time);
	u64 delta_ns = ktime_to_ns(delta);
	klog_info("took item to blocking queue, size=%d / %d, took %llu ns\n", size, QUEUE_CAPACITY, delta_ns);

	return true;
}