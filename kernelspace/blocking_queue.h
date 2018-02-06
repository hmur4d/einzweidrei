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

//Clears the content of the queue.
void blocking_queue_reset(void);

//Checks whether the queue is empty.
bool blocking_queue_is_empty(void);

//Adds a value to the end of the queue, wake up consumer if needed.
//Not blocking, will return false if the queue is full.
bool blocking_queue_add(int8_t value);

//Gets a value from the start of the queue and remove it.
//Blocking, will put the consumer to sleep if the queue is empty.
bool blocking_queue_take(int8_t* value);

#endif