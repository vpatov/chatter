#include "threadpool.h"
#include "debug.h"






pool_t *circular_pool_list;

int global_sum = 0;

thread_list_t *thread_list = NULL;

pthread_mutexattr_t work_mutex_attr;
pthread_mutex_t work_mutex;


void init_worker_mutex(){
	pthread_mutexattr_init(&work_mutex_attr);
	pthread_mutexattr_settype(&work_mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&work_mutex,&work_mutex_attr); 
}

void 
thread_cleanup()
{
	int ret;
	struct timespec lock_abstime;

	get_expiration_time(&lock_abstime,250);

	// int pthread_mutex_timedlock(pthread_mutex_t *restrict mutex, const struct timespec *restrict abs_timeout); 

	debug("Calling pthread_mutex_timedlock on work_mutex");
	ret = pthread_mutex_timedlock(&work_mutex, &lock_abstime);
	if (ret != 0){
		if (ret == ETIMEDOUT){
			debug("Caller doesn't own lock of work_mutex. Can continue exiting.");
		}
		else {
			error("pthread_mutex_timedlock of work_mutex returned a different error: %s", strerror(ret));
		}
	}
	else {
		debug("Callng pthread_mutex_unlock on work_mutex...");
		pthread_mutex_unlock(&work_mutex);
		debug("pthread_mutex_unlock of work_mutex - Success!");
	}

	debug("Calling pthread_mutex_timedlock on pool_mutex");
	ret = pthread_mutex_timedlock(&circular_pool_list->pool_mutex, &lock_abstime);
	if (ret != 0){
		if (ret == ETIMEDOUT){
			debug("Caller doesn't own lock of pool_mutex. Can continue exiting.");
		}
		else {
			error("pthread_mutex_timedlock of pool_mutex returned a different error: %s", strerror(ret));
		}
	}
	else {
		debug("Callng pthread_mutex_unlock on pool_mutex...");
		pthread_mutex_unlock(&circular_pool_list->pool_mutex);
		debug("pthread_mutex_unlock of pool_mutex- Success!");
	}


	///THIS DIDNT WORK VERY WELL AT ALL.
	// int ret;
	// debug("Trying to trylock work_mutex.");
	// ret = pthread_mutex_trylock(&work_mutex);
	// if (ret < 0){
	// 	error("pthread_mutex_trylock: %s",strerror(ret));
	// }
	// else {
	// 	debug("Trying to unlock work_mutex.");
	// 	pthread_mutex_unlock(&work_mutex);
	// }

	// debug("Trying to trylock pool_mutex.");
	// ret = pthread_mutex_trylock(&work_mutex);
	// if (ret < 0){
	// 	error("pthread_mutex_trylock: %s",strerror(ret));
	// }
	// else {
	// 	debug("Trying to unlock pool_mutex.");
	// 	pthread_mutex_unlock(&work_mutex);
	// }


//	info("Thread is leaving town.");
	pthread_exit(0);
}

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
	pool_t 	*pool, *temp;
	pthread_mutexattr_t mutex_attr;

	signal(SIGALRM,thread_cleanup);

	info("Creating a threadpool with min: %u, max: %u, linger: %u",min,max,linger);
	pool = malloc(sizeof(pool_t));


	//if this is the first pool being made, initialize the circular linked list.
	if (circular_pool_list == NULL){
		circular_pool_list = pool;
		circular_pool_list->pool_forw = pool;
		circular_pool_list->pool_back = pool;

	}
	//otherwise, tack it on to the circular list.
	else {
		temp = circular_pool_list->pool_back;
		temp->pool_forw = pool;
		circular_pool_list->pool_back = pool;
		pool->pool_forw = circular_pool_list;
		pool->pool_back = temp;
	}


	//initialize the pool mutex attributes for robustness
	//This is important because pool_destroy may cause some threads to
	//exit before they give up their pool_mutex lock, causing all the
	//other threads to deadlock. Either this will solve the problem,
	//or perhaps conditions?
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
	// pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);				
	pthread_mutex_init(&pool->pool_mutex, &mutex_attr);   /* initialize the mutex */


	//initialize the thread conditions

	r = pthread_cond_init(&pool->pool_busycv,NULL);
	if (r < 0){
		debug("%s",strerror(r));
		return NULL;
	}
	r = pthread_cond_init(&pool->pool_workcv,NULL);
	if (r < 0){
		debug("%s",strerror(r));
		return NULL;
	}
	r = pthread_cond_init(&pool->pool_waitcv,NULL);
	if (r < 0){
		debug("%s",strerror(r));
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

	
	r = pthread_mutex_lock(&pool->pool_mutex);									//acquire the lock before making changes to the pool structure
	if (r < 0){																	
		if (r == EOWNERDEAD){													//since we're using robust lock.
			pthread_mutex_consistent(&pool->pool_mutex);
		}
	}

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
			debug("%s",strerror(r));
			return r;
		}

		else {	

			add_to_thread_list(&thread);										//add the thread to the linked list of threads

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


void add_to_thread_list(pthread_t *thread){
	thread_list_t *node;

	node = malloc(sizeof(thread_list_t));
	node->thread = *thread;
	node->next = thread_list;
	thread_list = node;
}

void
get_expiration_time(struct timespec *abstime, uint16_t ms_delay)
{
	struct timeval current_time;

	gettimeofday(&current_time,NULL);
	abstime->tv_sec = current_time.tv_sec + (ms_delay / 1000);				//convert milliseconds to seconds
	abstime->tv_nsec = (current_time.tv_usec * 1000) + 							//convert remainder to nanoseconds
		((ms_delay % 1000) * 1000 * 1000);

}

int block_alarm(){
	sigset_t set;
	int s;

	/* Block SIGQUIT and SIGUSR1; other threads created by main()
	  will inherit a copy of the signal mask. */

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	s = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (s != 0){
	   error("block_alarm: pthread_sigmask: %s", strerror(s));
	}
	return s;
}

int unblock_alarm(){
	sigset_t set;
	int s;

	/* Block SIGQUIT and SIGUSR1; other threads created by main()
	  will inherit a copy of the signal mask. */

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	s = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	if (s != 0){
	   error("unblock_alarm: pthread_sigmask: %s", strerror(s));
	}
	return s;
}


void
*do_work(void *arg)
{
	int r;
	uint8_t restart_time, is_idle;
	pool_t *pool;
	job_t *job;
	thread_arg_t thread_arg;
	struct timespec linger_abstime;												//abs_time contains seconds and nanoseconds (10^1, 10^-9)




	pool = arg;
	job = NULL;
	restart_time = 1;
	is_idle = 0;
	thread_arg.pool = pool;




	while (1){
	//	block_alarm();															//block alarms
		pthread_mutex_lock(&pool->pool_mutex);									//lock before reading from job queue
		job = find_work(pool);													//look for work in job queue
		
		if (job != NULL){														//if we find a job
			restart_time = 1;													//make sure we restart the timer later
			if (is_idle){														//we found a job, we're no longer idle
				pool->pool_idle--;												//decrement the pool_idle counter
				is_idle = 0;
			}
			thread_arg.arg = job->job_arg;										//populate the thread_arg, which will hold pool and job_arg
			pthread_mutex_unlock(&pool->pool_mutex);							//other threads need to do work!
//			unblock_alarm();													//allow alarms

			debug("taking on job at %p", job);
			job->job_func(&thread_arg);											//call the work function of interest.
			free(job);															//upon work completion, free the job struct.
			success("job_func has returned inside of do_work.");
		}
		else {																	//if we don't find a job
			if (restart_time){													//if we are supposed to restart the timer
				get_expiration_time(&linger_abstime,pool->pool_linger);			//get the absolute time of timeout
				restart_time = 0;												//turn off restart flag
			}

			if (is_idle == 0){														//if we were not already idle, increment the idle counter
				pool->pool_idle++;												//and set is_idle to true. This way, if we were already idle
				is_idle = 1;													//we wouldn't double increment the pool_idle counter.
			}

//			unblock_alarm();
			r = pthread_cond_timedwait(&pool->pool_busycv,						//atomically release the lock, and block until timeout 
				&pool->pool_mutex,&linger_abstime);								//or signal on cond.
																	

			
			if (r == ETIMEDOUT){												//if we ran out of time, the thread should exit, unless
																				//there are too few threads
									
				if (pool->pool_nthreads <= pool->pool_minimum){					//if there are not enough threads, we gotta stay alive
					restart_time = 1;											//set the flag to restart the time
					pthread_mutex_unlock(&pool->pool_mutex);					//release the lock
//					unblock_alarm();
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
						// debug("Freeing worker %p with tid: %lx",
						// 	cursor, cursor->worker_tid);
						free(cursor);											//once upon a time we malloc'ed it, now we free it.
						break;													//break because we don't need to iterate anymore.
					}
					prev = cursor;												//update the cursor and prev references
					cursor = cursor->worker_next;
				}

				pool->pool_nthreads--;											//decrement the amount of nthreads
				pool->pool_idle--;												//before it expired, it was idle, by definition of our thread pool



				debug("This thread can't find any more jobs, "
					"and time has run out -- exiting.");

				pthread_mutex_unlock(&pool->pool_mutex);
//				unblock_alarm();
				pthread_exit(0);
				return NULL;													//haven't though of a use for this return value yet.
			}

			pthread_mutex_unlock(&pool->pool_mutex);							//we acquire the lock upon return from waiting, so release it.
																				//even though we are about to acquire it again, it is important to
																				//release it here so that we don't double lock, and thus block.
//			unblock_alarm();


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

void
pool_wait(pool_t *pool)
{
	uint8_t double_checking;

	double_checking = 0;
	while (1){
		pthread_mutex_lock(&pool->pool_mutex);
		if (pool->pool_idle == pool->pool_nthreads && pool->job_head == NULL){
			if (double_checking){
				debug("Main thread's pool_wait has double-checked, all queued jobs are done. Goodbye!");
				return;
			}
			double_checking = 1;
			debug("Main thread's pool_wait found no threads/jobs. Sleeping for 3 seconds before checking again...");
			pthread_mutex_unlock(&pool->pool_mutex);
			sleep(3);
		}
		else {
			debug("pool_idle :%u, nthreads: %u, job_head: %p", pool->pool_idle, pool->pool_nthreads, pool->job_head);
			debug("Main thread's pool_wait found threads/jobs. Sleeping for 5 seconds before checking again...");
			pthread_mutex_unlock(&pool->pool_mutex);
			sleep(5);
		}
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
	int ret, attempts;
	void *save;
	worker_t *worker;
	job_t *job;
	thread_list_t *node;
	struct timespec abstime;


	node = thread_list;
	while (node != NULL){
		attempts = 0;
		do {
			if (attempts == 2){
				break;
			}
			ret = pthread_kill(node->thread,SIGALRM);
			if (ret < 0){
				warn("pthread_kill returned non-zero.");
			}
			get_expiration_time(&abstime,200);
			debug("Attempt %d\tCalling pthread_timedjoin_np on thread: %lu", attempts,node->thread);
			ret = pthread_timedjoin_np(node->thread,NULL,&abstime);
			attempts++;
		} while(ret != 0);
		node = node->next;
	}

	debug("All threads joined in pool_destroy");
	
	worker = pool->pool_worker;
	while (worker != NULL){
		save = worker;
		worker = worker->worker_next;
		free(save);
	}
	job = pool->job_head;
	while (job != NULL){
		save = job;
		job = job->job_next;
		free(save);
	}


	node = thread_list;
	while (node != NULL){
		save = node;
		node = node->next;
		free(save);
	}
	thread_list = NULL;

	pool->job_head = NULL;
	pool->job_tail = NULL;

	pthread_mutex_destroy(&pool->pool_mutex);


	//remove from circular linked list
	//if there is only one pool (it is the only pool)
	if (circular_pool_list->pool_forw == circular_pool_list){
		circular_pool_list = NULL;
	}
	else {
		//connect pool's previous to its next, and vice versa
		pool->pool_forw->pool_back = pool->pool_back;
		pool->pool_back->pool_forw = pool->pool_forw;

		if (pool == circular_pool_list){
			circular_pool_list = pool->pool_forw; // could go back or forward, abritrary
		}
	}

	//free it
	free(pool);
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


void *test_routine2(void *arg){
	// thread_arg_t *thread_arg = arg;
	// pool_t *pool = thread_arg->pool;

	int i,x;
	x = 5;
	for (i = 0; i < 10000000; i++){
		if (i % 10000 == 0){
			pthread_mutex_lock(&work_mutex);
			global_sum += 1;
			pthread_mutex_unlock(&work_mutex);
		}
		x = (sqrt(i * x));
	}
	return NULL;
}



int main(){
	int args[10];
	int i,a;
	pool_t *pool;

	info("Starting threadpool tests....");

	init_worker_mutex();

	 for (a = 0; a < 1; a++){
	 	pool = pool_create(5, 400, 300, NULL);
	 	for (i = 0; i < 500; i++){
	 		args[i] = i;
	 		pool_queue(pool,test_routine2, &args[i]);

	 	}
	 	sleep(1);
	 	pool_destroy(pool);
	 	info("============================== "
	 		 "Pool has been destroyed by pool_destroy. "
	 		 "============================");
	 	info("global_sum is :%d", global_sum);
	 }

/*
	pool = pool_create(5, 70, 300, NULL);
	for (i = 0; i < 300; i++){
		args[i] = i;
		pool_queue(pool,test_routine2, &args[i]);

	}
	pool_wait(pool);
	info("global_sum is :%d", global_sum);
*/
	sleep(20);
	exit(0);

}
