/*	$NetBSD: mutex4.c,v 1.1 2003/01/30 18:05:27 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex;

int
main(int argc, char *argv[])
{
	int x,ret;
	pthread_t new;
	pthread_mutexattr_t mattr;
	void *joinval;

	printf("1: Mutex-test 4\n");

	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&mutex, &mattr);

	pthread_mutexattr_destroy(&mattr);

	x = 1;
	pthread_mutex_lock(&mutex);
	ret = pthread_create(&new, NULL, threadfunc, &x);
	if (ret != 0)
		err(1, "pthread_create");

	printf("1: Before recursively acquiring the mutex.\n");
	ret = pthread_mutex_lock(&mutex);
	if (ret != 0)
		err(1, "pthread_mutex_lock recursive");

	printf("1: Before releasing the mutex once.\n");
	sleep(2);
	pthread_mutex_unlock(&mutex);
	printf("1: After releasing the mutex once.\n");

	x = 20;

	printf("1: Before releasing the mutex twice.\n");
	sleep(2);
	pthread_mutex_unlock(&mutex);
	printf("1: After releasing the mutex twice.\n");

	ret = pthread_join(new, &joinval);
	if (ret != 0)
		err(1, "pthread_join");

	pthread_mutex_lock(&mutex);
	printf("1: Thread joined. X was %d. Return value (int) was %d\n", 
	    x, *(int *)joinval);
	assert(x == 21);
	assert(*(int *)joinval == 21);
	pthread_mutex_unlock(&mutex);
	return 0;
}

void *
threadfunc(void *arg)
{
	int *param;

	printf("2: Second thread.\n");

	param = arg;
	printf("2: Locking mutex\n");
	pthread_mutex_lock(&mutex);
	printf("2: Got mutex. *param = %d\n", *param);
	(*param)++;

	pthread_mutex_unlock(&mutex);

	return param;
}
