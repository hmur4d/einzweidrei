#ifndef _BLOCKING_QUEUE_H
#define _BLOCKING_QUEUE_H

/*
Defines a blocking queue, using a fifo with a wait queue.
This queue will block when calling blocking_queue_take(..) on an empty queue.
It will not block when trying to add a value to a full queue, but will report an error instead.
*/

#include "linux_includes.h"

//must be power of 2
#define FIFO_SIZE 16

typedef struct {
	wait_queue_head_t waitqueue;
	DECLARE_KFIFO(fifo, int8_t, FIFO_SIZE);
	long last_push;
} blocking_queue_t;

//Clears the content of the queue.
void blocking_queue_reset(blocking_queue_t* queue);

//Checks whether the queue is empty.
bool blocking_queue_is_empty(blocking_queue_t* queue);

//Adds a value to the end of the queue, wake up consumer if needed.
//Not blocking, will return false if the queue is full.
bool blocking_queue_add(blocking_queue_t* queue, int8_t value);

//Gets a value from the start of the queue and remove it.
//Blocking, will put the consumer to sleep if the queue is empty.
bool blocking_queue_take(blocking_queue_t* queue, int8_t* value);

#endif