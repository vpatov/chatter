#include "threadpool.h"

/**
* Creates a thread pool. More than one pool can be created.
* @param min Minimum number of threads in the pool.
* @param max Maximum number of threads in the pool.
* @param linger Number of seconds that idle threads can linger before* exiting, when no tasks come in. The idle threads can only exit if
* they are extra threads, above the number of minimum threads.
* @param attr Attributes of all worker threads. This can be NULL.
* @return Returns a pool_t if the operation was successful. On error,
* NULL is returned with errno set to the error code.
*/

pool_t *global_pool_head;
pool_t *global_pool_tail;

pool_t*
pool_create(uint16_t min, uint16_t max, uint16_t linger, pthread_attr_t* attr)
{
	int r;
	pool_t 	*pool;

	pool = malloc(sizeof(pool_t));
	if (global_pool_head == NULL){
		global_pool_head = pool;

	}

	//append it to the linked list
	pool->pool_back = global_pool_tail;
	global_pool_tail = pool;
	pool->pool_forw = NULL;
	
	//initialize the mutex lock
	r = pthread_mutex_init(&pool->pool_mutex,NULL);
	if (r < 0){
		strerror(errno);
		return NULL;
	}


	//initialize the thread conditions
	// TODO
	// 	pthread_cond_t pool_busycv; 		/* synchronization in pool_queue */
	// 	pthread_cond_t pool_workcv; 		/* synchronization with workers */
	// 	pthread_cond_t pool_waitcv; 		/* synchronization in pool_wait() */

	//initialize the active pool list.
	pool->pool_worker = NULL;

	//initialize the queue of jobs.
	pool->job_head = NULL;
	pool->job_tail = NULL;

	//initialize the attributes of the workers.
	memcpy(attr,&(pool->pool_attr),sizeof(pthread_attr_t));

	//initialize the rest of the members
	pool->pool_flags = 0;
	pool->pool_linger = linger;
	pool->pool_minimum = min;
	pool->pool_maximum = max;
	pool->pool_nthreads = 0;
	pool->pool_idle = 0;

	return pool;



// typedef struct th_pool pool_t;
// struct th_pool {
// 	pool_t *pool_forw; 					/* circular linked list */
// 	pool_t *pool_back; 					/* of all thread pools */
// 	pthread_mutex_t pool_mutex; 		/* protects the pool data */
// 	pthread_cond_t pool_busycv; 		/* synchronization in pool_queue */
// 	pthread_cond_t pool_workcv; 		/* synchronization with workers */
// 	pthread_cond_t pool_waitcv; 		/* synchronization in pool_wait() */
// 	worker_t *pool_worker; 				/* list of threads performing work */
// 	job_t *job_head;					/* head of FIFO job queue */
// 	job_t *job_tail; 					/* tail of FIFO job queue */
// 	pthread_attr_t pool_attr; 			/* attributes of the workers */
// 	int pool_flags; 					/* see below */
// 	uint16_t pool_linger;				/* seconds before idle workers exit */
// 	uint16_t pool_minimum;				/* min number of worker threads */
// 	uint16_t pool_maximum;				/* max number of worker threads */
// 	uint16_t pool_nthreads; 			/* current number of worker threads */
// 	uint16_t pool_idle; 				/* number of idle workers */
// };


// typedef struct worker worker_t;
// struct worker {
// 	worker_t *worker_next;
// 	uint8_t is_active;
// 	pthread_t worker_tid; 	
// };


}



/*
* Enqueue a work request to the thread pool job queue.
* If there are idle worker threads, awaken one to perform the job.
* Else if the maximum number of workers has not been reached,
* create a new worker thread to perform the job.
* Else just return after adding the job to the queue;
* an existing worker thread will perform the job when
* it finishes the job it is currently performing.
* @param pool A thread pool identifier returned from pool_create().
* @param func The task function to be called.
* @param arg The only argument passed to the task function.
* @return Returns 0 on success, otherwise -1 with errno set to the error code.
*/

int
pool_queue(pool_t* pool, void* (*func)(void *), void* arg)
{
	int r = 0;
	job_t *job;
	worker_t *worker;


	// ----------- Enqueue a work request to the thread pool job queue. ----------- //

	//allocate memory for the job
	job = malloc(sizeof(job_t));
	job->job_next = NULL;
	job->job_func = func;
	job->job_arg = arg;

	//add it to the pool's linked list
	//probably has to be synchronized with conditions
	if (pool->job_tail == NULL){
		pool->job_head = job;
		pool->job_tail = job;
	}
	else {
		pool->job_tail->job_next = job;
	}
	pool->job_tail = job;

// 	// ----------- If there are idle worker threads, awaken one to perform the job.
// * Else if the maximum number of workers has not been reached,
// * create a new worker thread to perform the job.
// * Else just return after adding the job to the queue;

	if (pool->pool_idle){
		worker = pool->pool_worker;
		while (worker != NULL && worker->is_active){ 		//boolean expression short-circuit evaluation prevents dereference of NULL pointer
			worker = worker->worker_next;
		}
	}

	return r;
}




/**
* Wait for all queued jobs to complete in the thread pool.
* @param pool A thread pool identifier returned from pool_create().
**/

void
pool_wait(pool_t *pool)
{

}


/**
* Cancel all queued jobs and destroy the pool. Worker threads that are
* actively processing tasks are cancelled.
* @param pool A thread pool identifier returned from pool_create().
**/
void
pool_destroy(pool_t *pool)
{

}


int main(){

}