/*	$NetBSD: cond6.c,v 1.1 2004/12/10 17:07:31 nathanw Exp $	*/

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int main(void)
{
	int ret;
	pthread_t new;
	struct timespec ts;
	struct timeval tv;

	printf("condition variable test 6: bogus timedwait\n");

	ret = pthread_mutex_lock(&mutex);
	if (ret)
		err(1, "pthread_mutex_lock(1)");

	printf("unthreaded test\n");
	gettimeofday(&tv, NULL);
	tv.tv_sec -= 2; /* Place the time in the past */
	TIMEVAL_TO_TIMESPEC(&tv, &ts);

	ret = pthread_cond_timedwait(&cond, &mutex, &ts);
	if (ret != ETIMEDOUT) {
		printf("FAIL: pthread_cond_timedwait() (unthreaded)"
		    " in the past returned %d\n", ret);
		exit(1);
	}
		
	ret = pthread_create(&new, NULL, threadfunc, NULL);
	if (ret != 0)
		err(1, "pthread_create");
	ret = pthread_join(new, NULL);
	if (ret != 0)
		err(1, "pthread_join");

	printf("threaded test\n");
	gettimeofday(&tv, NULL);
	tv.tv_sec -= 2; /* Place the time in the past */
	TIMEVAL_TO_TIMESPEC(&tv, &ts);
	
	ret = pthread_cond_timedwait(&cond, &mutex, &ts);
	if (ret != ETIMEDOUT) {
		printf("FAIL: pthread_cond_timedwait() (threaded)"
		    " in the past returned %d\n", ret);
		exit(1);
	}

	pthread_mutex_unlock(&mutex);

	return 0;
}

void *
threadfunc(void *arg)
{
	return NULL;
}
