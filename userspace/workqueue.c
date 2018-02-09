#include "workqueue.h"
#include "log.h"

typedef struct work_node {
	worker_f worker;
	void* data;
	cleanup_f cleanup;

	struct work_node* next;
} workitem_t;

//using a double ended queue for faster adds
//this will allow interrupt reader to submit tasks faster
typedef struct {
	workitem_t* first;
	workitem_t* last;
} workqueue_t;

//--

static workqueue_t queue;
static pthread_mutex_t mutex;
static pthread_t thread;
static pthread_cond_t not_empty_condition;
static pthread_mutex_t not_empty_mutex;

static bool initialized = false;

//--

static workitem_t* create_workitem(worker_f worker, void* data, cleanup_f cleanup) {
	workitem_t* item = malloc(sizeof(workitem_t));
	if (item == NULL) {
		log_error_errno("Unable to malloc work item!");
		return NULL;
	}

	item->worker = worker;
	item->data = data;
	item->cleanup = cleanup;
	item->next = NULL;
	return item;
}

static void destroy_workitem(workitem_t* item) {
	if (item->data != NULL && item->cleanup != NULL) {
		//free contained data, using cleanup function.
		log_debug("Calling cleanup function...");
		item->cleanup(item->data);
	}

	free(item);
}

//--

static bool is_empty() {
	return queue.first == NULL;
}

static workitem_t* pop_first() {
	pthread_mutex_lock(&mutex);
	if (is_empty()) {
		return NULL;
		pthread_mutex_unlock(&mutex);
	}

	workitem_t* first = queue.first;
	queue.first = first->next;

	if (queue.last == first) {
		//there was only one item, mark the last one as removed too
		queue.last = NULL;
	}

	pthread_mutex_unlock(&mutex);
	return first;
}

static bool exec_first() {
	workitem_t * item = pop_first();
	if (item == NULL) {
		log_error("Empty work queue!");
		return false;
	}

	log_debug("Calling worker...");
	item->worker(item->data);
	destroy_workitem(item);
	return true;
}

static void* workqueue_thread(void* data) {
	while (true) {
		//wait condition, signaled from workqueue_submit()
		pthread_mutex_lock(&not_empty_mutex);
		while(is_empty()) {
			pthread_cond_wait(&not_empty_condition, &not_empty_mutex);
		}	
		pthread_mutex_unlock(&not_empty_mutex);

		exec_first();
	}

	return NULL;
}

//--

bool workqueue_init() {
	log_debug("Creating workqueue mutex");
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		log_error("Unable to init mutex");
		return false;
	}

	log_debug("Creating workqueue condition & mutex");
	if (pthread_cond_init(&not_empty_condition, NULL) != 0) {
		log_error("Unable to create workqueue condition!");
		return false;
	}

	if (pthread_mutex_init(&not_empty_mutex, NULL) != 0) {
		log_error("Unable to init workqueue condition mutex");
		return false;
	}

	log_debug("Creating workqueue thread");
	if (pthread_create(&thread, NULL, workqueue_thread, NULL) != 0) {
		log_error("Unable to create workqueue thread!");
		return false;
	}


	queue.first = NULL;
	queue.last = NULL;
	initialized = true;
	return true;
}

bool workqueue_destroy() {
	if (pthread_cancel(thread) != 0) {
		log_error("Unable to cancel workqueue thread");
		return false;
	}

	if (pthread_join(thread, NULL) != 0) {
		log_error("Unable to join workqueue thread");
		return false;
	}

	if (pthread_cond_destroy(&not_empty_condition) != 0) {
		log_error("Unable to destroy workqueue condition!");
		return false;
	}

	if (pthread_mutex_destroy(&not_empty_mutex) != 0) {
		log_error("Unable to destroy workqueue condition mutex!");
		return false;
	}

	while (!is_empty()) {
		workitem_t* item = pop_first();
		destroy_workitem(item);
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		log_error("Unable to destroy mutex");
		return false;
	}

	initialized = false;
	return true;
}

bool workqueue_submit(worker_f worker, void* data, cleanup_f cleanup) {
	if (!initialized) {
		log_error("Trying to submit a worker before initialization!");
		return false;
	}

	//lock first to ensure execution ordering. 
	//We don't want to risk worker 2 to run before worker 1.
	pthread_mutex_lock(&mutex);

	workitem_t* item = create_workitem(worker, data, cleanup);
	if (item == NULL) {
		pthread_mutex_unlock(&mutex);
		return false;
	}

	if (queue.last == NULL) {
		//first job, set as first and last
		queue.first = item;
		queue.last = item;
	}
	else {
		//add job after last
		queue.last->next = item;
		queue.last = item;
	}

	pthread_mutex_unlock(&mutex);

	log_debug("submitted new worker");
	pthread_cond_signal(&not_empty_condition);
	return true;
}