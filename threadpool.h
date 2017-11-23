#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

/* pool_flags */
#define POOL_WAIT 0x01 					/* waiting in thr_pool_wait() */
#define POOL_DESTROY 0x02				/* pool is being destroyed */



typedef struct job job_t;
struct job {
	job_t
	*job_next;							/* linked list of jobs */
	void *(*job_func)(void *);			/* function to call */
	void *job_arg;						/* its argument */
};



typedef struct active active_t;
struct active {
	active_t *active_next; 				/* linked list of threads */
	pthread_t active_tid;				/* active thread id */
};



typedef struct th_pool pool_t;
struct th_pool {
	pool_t *pool_forw; 					/* circular linked list */
	pool_t *pool_back; 					/* of all thread pools */
	pthread_mutex_t pool_mutex; 		/* protects the pool data */
	pthread_cond_t pool_busycv; 		/* synchronization in pool_queue */
	pthread_cond_t pool_workcv; 		/* synchronization with workers */
	pthread_cond_t pool_waitcv; 		/* synchronization in pool_wait() */
	active_t *pool_active; 				/* list of threads performing work */
	job_t *job_head;					/* head of FIFO job queue */
	job_t *job_tail; 					/* tail of FIFO job queue */
	pthread_attr_t pool_attr; 			/* attributes of the workers */
	int pool_flags; 					/* see below */
	uint16_t pool_linger;				/* seconds before idle workers exit */
	uint16_t pool_minimum;				/* min number of worker threads */
	uint16_t pool_maximum;				/* max number of worker threads */
	uint16_t pool_nthreads; 			/* current number of worker threads */
	uint16_t pool_idle; 				/* number of idle workers */
};


/* Function Prototypes */
pool_t* 	pool_create(uint16_t min, uint16_t max, uint16_t linger, pthread_attr_t* attr);
int 		pool_queue(pool_t* pool, void* (*func)(void *), void* arg);
void		pool_wait(pool_t *pool);
void		pool_destroy(pool_t *pool);