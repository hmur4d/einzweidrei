#ifndef _WORKQUEUE_H_
#define _WORKQUEUE_H_

/*
Asynchronous execution of code using a work-queue
*/

#include "std_includes.h"

//Worker function
typedef void(*worker_f) (void* data);

//Cleanup function, called after execution or on shutdown.
//Can simply be "free()"
typedef void(*cleanup_f) (void* data);

//--

//Initialize the work queue, start its thread.
//Must be done before submitting any worker.
bool workqueue_start();

//Stops threads and cleanup.
//If there are still workers in queue, they will not be processed. 
//The cleanup function will still be called.
bool workqueue_stop();

//Submit a new worker. The data & cleanup function can be NULL if the worker doesn't need any data.
bool workqueue_submit(worker_f worker, void* data, cleanup_f cleanup);

#endif

