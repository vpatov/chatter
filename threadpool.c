#include "threadpool.h"
#include "debug.h"






pool_t *global_pool_head;
pool_t *global_pool_tail;


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
		debug("%s",strerror(errno));
		return NULL;
	}
	r = pthread_mutex_init(&pool->pool_mutex,NULL);
	if (r < 0){
		debug("%s",strerror(errno));
		return NULL;
	}



	//initialize the thread conditions

	r = pthread_cond_init(&pool->pool_busycv,NULL);
	if (r < 0){
		debug("%s",strerror(errno));
		return NULL;
	}
	r = pthread_cond_init(&pool->pool_workcv,NULL);
	if (r < 0){
		debug("%s",strerror(errno));
		return NULL;
	}
	r = pthread_cond_init(&pool->pool_waitcv,NULL);
	if (r < 0){
		debug("%s",strerror(errno));
		return NULL;
	}

	//initialize the active pool list.
	pool->pool_worker = NULL;

	//initialize the queue of jobs.
	pool->job_head = NULL;
	pool->job_tail = NULL;


	//initialize the attributes of the workers.
	if (attr != NULL){
		memcpy(attr,&pool->pool_attr,sizeof(pthread_attr_t));
	}
	else {
		pthread_attr_init(&pool->pool_attr);
	}


	//initialize the rest of the members
	pool->pool_flags = 0;
	pool->pool_linger = linger;
	pool->pool_minimum = min;
	pool->pool_maximum = max;
	pool->pool_nthreads = 0;
	pool->pool_idle = 0;

	return pool;


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
	pthread_t thread;

	/*
		Enqueue a work request to the thread pool job queue.
	*/

	job = malloc(sizeof(job_t));												//allocate memory for the job
	job->job_next = NULL;
	job->job_func = func;
	job->job_arg = arg;

	
	pthread_mutex_lock(&pool->pool_mutex);										//acquire the lock before making changes to the pool structure

	if (pool->job_tail == NULL){												//if the list is empty, init the head
		pool->job_head = job;
	}																			
	else {
		pool->job_tail->job_next = job;											//if not empty, just tack it onto the end.
	}
	pool->job_tail = job;														//point the tail to the end


	if (pool->pool_idle){														//if the pool has idle workers, signal one to do work
		pthread_cond_signal(&pool->pool_busycv);	 							//use the pool_busycv condition
	}


	else if (pool->pool_nthreads < pool->pool_maximum){							//if the pool has no idle workers, but can support more threads

		r = pthread_create(&thread,&pool->pool_attr,do_work,pool);				//make a new thread, and assign it the template worker routine.
		if (r < 0){
			debug("%s",strerror(errno));
			return r;
		}
		else {	
			pool->pool_nthreads++;												//increment the number of threads

			worker = malloc(sizeof(worker_t));									//allocate memory for a new worker_t
			worker->worker_next = NULL;											//initialize worker_next (NULL at first)
			worker->worker_tid = thread;										//initialize worker_tid to the thread filled in by pthread_create

			if (pool->pool_worker != NULL){										//put this new thread on the top of the stack.
				worker->worker_next = pool->pool_worker;
			}
			pool->pool_worker = worker;											

		}

	}

	else {
		//info("Maximum number of threads has been reached. Adding onto job queue...");
	}

	pthread_mutex_unlock(&pool->pool_mutex);									//release the lock after the changes to the pool have been made.


	return r;
}



void
get_expiration_time(struct timespec *abstime, uint16_t pool_linger)
{
	struct timeval current_time;

	gettimeofday(&current_time,NULL);
	abstime->tv_sec = current_time.tv_sec + (pool_linger / 1000);				//convert milliseconds to seconds
	abstime->tv_nsec = (current_time.tv_usec * 1000) + 							//convert remainder to nanoseconds
		((pool_linger % 1000) * 1000 * 1000);

}



void
*do_work(void *arg)
{
	int r;
	uint8_t restart_time;
	pool_t *pool;
	job_t *job;

	struct timespec abstime;													//abs_time contains seconds and nanoseconds (10^1, 10^-9)


	pool = arg;
	job = NULL;
	restart_time = 1;

	while (1){
		pthread_mutex_lock(&pool->pool_mutex);									//lock before reading from job queue
		job = find_work(pool);													//look for work in job queue
		
		if (job != NULL){														//if we find a job
			restart_time = 1;													//make sure we restart the timer later
			pthread_mutex_unlock(&pool->pool_mutex);							//other threads need to do work!
			job->job_func(job->job_arg);								//call the work function of interest.
			free(job);															//upon work completion, free the job struct.
			success("job_func has returned inside of do_work.");
		}
		else {
			if (restart_time){													//if we are supposed to restart the timer
				get_expiration_time(&abstime,pool->pool_linger);				//get the absolute time of timeout
				restart_time = 0;												//turn off restart flag
			}

			pool->pool_idle++;													//this thread is about to be waiting, it is idle

			r = pthread_cond_timedwait(&pool->pool_busycv,						//atomically release the lock, and block until timeout 
				&pool->pool_mutex,&abstime);									//or signal on cond.
																	
			pool->pool_idle--;													//a thread is only idle if it is waiting in timedwait
			
			if (r == ETIMEDOUT){												//if we ran out of time, the thread should exit, unless
																				//there are too few threads
									
				if (pool->pool_nthreads <= pool->pool_minimum){					//if there are not enough threads, we gotta stay alive
					restart_time = 1;											//set the flag to restart the time
					pthread_mutex_unlock(&pool->pool_mutex);					//release the lock
					continue;													//go back to try to find work
				}

				pthread_t self_tid;												
				worker_t *cursor, *prev;	

				self_tid = pthread_self();										//get the pthread_id.
				cursor = pool->pool_worker;
				prev = NULL;

				while (cursor != NULL){											//find where in the linked list the thread is, and remove it.
					if (cursor->worker_tid == self_tid){						//if the pthread_id's are the same
						if (prev != NULL){										//if the cursor is not the front
							prev->worker_next = cursor->worker_next;			//cut the node and tie together the separated ends
						}
						else {
							pool->pool_worker = cursor->worker_next;			//if at the front, just cut the front.
						}
						debug("Freeing worker %p with tid: %lx",cursor, cursor->worker_tid);
						free(cursor);											//once upon a time we malloc'ed it, now we free it.
						break;													//break because we don't need to iterate anymore.
					}
					prev = cursor;												//update the cursor and prev references
					cursor = cursor->worker_next;
				}

				pool->pool_nthreads--;


				debug("This thread can't find any more jobs, "
					"and time has run out -- exiting.");

				pthread_mutex_unlock(&pool->pool_mutex);
				return NULL;													//haven't though of a use for this return value yet.
			}

			pthread_mutex_unlock(&pool->pool_mutex);							//we acquire the lock upon return from waiting, so release it.
																				//even though we are about to acquire it again, it is important to
																				//release it here so that we don't double lock, and thus block.


			/* 
				If we didn't run out of time, then a signal woke us up. Supposedly, only one thread 
				can be woken up by a signal at a time. However, in the case that we are woken up, and 
				it turns out that there are no jobs, we still have to go back to being idle. The timer 
				is not reset in this instance, thus the thread won't end up being idle for longer if it
				was woken up for no good reason. 
			*/
			
		}
	}	

	


}


job_t
*find_work(pool_t *pool)
{
	job_t *save;

	if (pool->job_head != NULL) {												// if the list is not empty
		save = pool->job_head;													// save the reference to the head
		pool->job_head = pool->job_head->job_next;								// set the head to the next job
		if (pool->job_tail == save)												// if there was only one job
			pool->job_tail = NULL;												// set the tail to NULL as well
		return save;															// return the job we popped
	}
	return NULL;																// if no jobs, return NULL

	
}


/**
* Wait for all queued jobs to complete in the thread pool.
* @param pool A thread pool identifier returned from pool_create().
**/

// void
// pool_wait(pool_t *pool)
// {
// 	job_t *job;
// 	uint8_t double_checking;

// 	double_checking = 0;
// 	while (1){
// 		pthread_mutex_lock(&pool->pool_mutex);
// 		job = pool->job_head;
// 		if (job == NULL){
// 			if (double_checking){
// 				debug("Main thread's pool_wait has double-checked, all jobs are done. Goodbye!");
// 				exit(0);
// 			}
// 			double_checking = 1;
// 			debug("Main thread's pool_wait found no jobs in the queue. Sleeping for 3 seconds before checking again...");
// 			pthread_mutex_unlock(&pool->pool_mutex);
// 			sleep(3);
// 		}
// 		else {
// 			debug("Main thread's pool_wait found jobs still in queue. Sleeping for 10 seconds before checking again...");
// 			pthread_mutex_unlock(&pool->pool_mutex);
// 			sleep(10);
// 		}
// 	}
// }




void 
pool_wait(pool_t *pool)
{
	worker_t *worker;
	void *retval;
	// pthread_mutex_lock(&pool->pool_mutex);
	worker = pool->pool_worker;
	// pthread_mutex_unlock(&pool->pool_mutex);
	while(worker != NULL){

		debug("Waiting for worker: %p with tid: %lx ",worker, worker->worker_tid);
		pthread_join(worker->worker_tid,&retval);
		debug("Joined worker: %p with tid: %lx", worker, worker->worker_tid);
		// pthread_mutex_lock(&pool->pool_mutex);
		worker = worker->worker_next;
		// pthread_mutex_unlock(&pool->pool_mutex);
	}

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


char *get_color(int i){
	char *color;
	i = i % 8;
	switch(i){
		case(0): color = KRED; break;
		case(1): color = KGRN; break;
		case(2): color = KYEL; break;
		case(3): color = KBLU; break;
		case(4): color = KMAG; break;
		case(5): color = KCYN; break;
		case(6): color = KWHT; break;
		case(7): color = KBWN; break;
		default: color = KNRM; break;
	}
	return color;
}

void *test_routine(void *arg){
	int i, j, x;
	char *color;
	color = get_color(*(int *)(arg));
	for (i = 0 ; i < 5; i++){
		printf("%s Routine %d: %d\n" KNRM,color,*(int *)(arg), i );
		for (j = 0; j < 10000000; j++){
			//some timely computation
			x = (i * j) * ((66444564 * i) + j);
			x = (x * 45) + x * 11;
			x = sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(x)))))));
		}
	}
	info("test_routine %d done.", *(int *)(arg));
	return NULL;
}





int main(){
	int args[10];
	int i;
	pool_t *pool;


	
	pool = pool_create(3, 50, 100, NULL);
	for (i = 0; i < 5; i++){
		args[i] = i;
		pool_queue(pool,test_routine, &args[i]);

	}

	pool_wait(pool);
	// while (1){
	// 	info("Main thread:\nnumthreads: %d, idle threads: %d\n",pool->pool_nthreads,pool->pool_idle);

	// 	worker_t *worker;
	// 	i = 0;
	// 	worker = pool->pool_worker;
	// 	while (worker != NULL){
	// 		info("Found worker: %d\n", i++);
	// 		worker = worker->worker_next;
	// 	}
	// 	sleep(10);
	// }


}