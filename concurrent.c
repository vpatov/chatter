#include "concurrent.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>





int
lock(pthread_mutex_t *mutex)
{
	int r;
	char error_msg[MAX_ERROR_SIZE];
	r = pthread_mutex_lock(mutex);									//acquire the lock before making changes to the pool structure
	if (r < 0){																	
		if (r == EOWNERDEAD){										//since we're using robust lock.
			pthread_mutex_consistent(mutex);
		}
		else {	
			strerror_r(r,error_msg,MAX_ERROR_SIZE);
			fprintf(stderr,"pthread_mutex_lock returned an error: %s",error_msg);

		}
	}
	return r;
}

int unlock(pthread_mutex_t *mutex)
{
	int r;
	char error_msg[MAX_ERROR_SIZE];
	r = pthread_mutex_unlock(mutex);
	if (r < 0){																	
		strerror_r(r,error_msg,MAX_ERROR_SIZE);
		fprintf(stderr,"pthread_mutex_unlock returned an error: %s",error_msg);
	}
	return r;
}
