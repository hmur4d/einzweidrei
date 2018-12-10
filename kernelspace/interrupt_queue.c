#include "interrupt_queue.h"
#include "klog.h"

typedef struct {
	int8_t value;
	ktime_t time;
} timed_value_t;

//use a ring buffer
static int size = 0;
static int next_read = 0;
static int next_write = 0;
static timed_value_t buffer[INTERRUPT_QUEUE_CAPACITY];

//count empty take to allow waiting longer when the buffer remains empty for many calls
static ulong empty_takes = 0;

void interrupt_queue_reset(void) {
	size = 0;
	next_read = 0;
	next_write = 0;
	empty_takes = 0;
}

bool interrupt_queue_is_empty(void) {
	return size == 0;
}

bool interrupt_queue_add(uint8_t value) {
	if (size + 1 == INTERRUPT_QUEUE_CAPACITY) {
		klog_warning("Unable to add item to interrupt queue, size=%d\n", size);
		return false;
	}

	timed_value_t timedvalue = { .value = value, .time = ktime_get() };
	buffer[next_write] = timedvalue;
	size++;

	next_write = (next_write + 1) % INTERRUPT_QUEUE_CAPACITY;
	return true;
}

bool interrupt_queue_take(uint8_t* value) {	
	if (size == 0) {
		empty_takes++;

		//select a waiting strategy depending of the queue activity.
		if (empty_takes < 100000) {
			//busy, return early to limit latency
			return false;
		} else if (empty_takes < 1000000) {
			//somewhat busy, but not that much, allow a small delay
			usleep_range(0, 100);
		} else {
			//waiting, let cpu sleep a bit
			usleep_range(100, 1000);
		}

		if (size == 0) {
			//still empty, returns
			return false;
		}
	}

	timed_value_t timedvalue = buffer[next_read];
	next_read = (next_read + 1) % INTERRUPT_QUEUE_CAPACITY;
	*value = timedvalue.value;
	size--;
	
	/*
	ktime_t now = ktime_get();
	ktime_t delta = ktime_sub(now, timedvalue.time);
	u64 delta_ns = ktime_to_ns(delta);
	if (delta_ns > 1000000) {
		klog_warning("latency from interrupt_queue_take: size=%d / %d, time=%llu ns\n", size, INTERRUPT_QUEUE_CAPACITY, delta_ns);
	}
	*/

	empty_takes = 0;
	return true;
}