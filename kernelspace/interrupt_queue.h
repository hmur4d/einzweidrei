#ifndef _INTERRUPT_QUEUE_H
#define _INTERRUPT_QUEUE_H

/*
Defines a queue, using a ring buffer to store values.
This queue will return immediately when calling interrupt_queue_take(..) on a non-empty queue.
When interrupt_queue_take(..) is called on an empty queue, it may either return immediately
or wait a bit and see if some value arrives, depending on its usage rate.

It will not block when trying to add a value to a full queue, but will report an error instead.
*/

#include "linux_includes.h"

#define INTERRUPT_QUEUE_CAPACITY 16

//Clears the content of the queue.
void interrupt_queue_reset(void);

//Checks whether the queue is empty.
bool interrupt_queue_is_empty(void);

//Checks whether the queue contains a specific interrupt
bool interrupt_queue_contains(uint8_t value);

//Adds a value to the end of the queue, wake up consumer if needed.
//Not blocking, will return false if the queue is full.
bool interrupt_queue_add(uint8_t value);

//Gets a value from the start of the queue and remove it.
//May wait a bit if the queue is empty.
bool interrupt_queue_take(uint8_t* value);

//Asks for queue status, used for logging only.
void interrupt_queue_status(int *qsize, int *qnext_read, int *qnext_write, ulong *qempty_takes);

#endif