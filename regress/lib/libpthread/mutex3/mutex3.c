/*	$NetBSD: mutex3.c,v 1.1 2003/01/30 18:05:26 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int x;

int
main(int argc, char *argv[])
{
	int count, count2, ret;
	pthread_t new;
	void *joinval;

	printf("1: Mutex-test 3\n");

	x = 0;
	count = count2 = 10000000;
	ret = pthread_mutex_lock(&mutex);
	if (ret != 0)
		err(1, "pthread_mutex_lock (1)");
	ret = pthread_create(&new, NULL, threadfunc, &count2);
	if (ret != 0)
		err(1, "pthread_create");

	printf("1: Thread %p\n", pthread_self());
	ret = pthread_mutex_unlock(&mutex);
	if (ret != 0)
		err(1, "pthread_mutex_unlock(1)");


	while (count--) {
		ret = pthread_mutex_lock(&mutex);
		if (ret != 0)
			err(1, "pthread_mutex_lock(2)");
		x++;
		ret = pthread_mutex_unlock(&mutex);
		if (ret != 0)
			err(1, "pthread_mutex_unlock(2)");
	}

	ret = pthread_join(new, &joinval);
	if (ret != 0)
		err(1, "pthread_join");

	pthread_mutex_lock(&mutex);
	printf("1: Thread joined. X was %d. Return value (long) was %ld\n", 
	    x, (long)joinval);
	assert(x == 20000000);
	return 0;
}

void *
threadfunc(void *arg)
{
	int count = *(int *)arg;
	int ret;

	printf("2: Second thread (%p). Count is %d\n", pthread_self(), count);

	while (count--) {
		ret = pthread_mutex_lock(&mutex);
		if (ret != 0)
			err(1, "pthread_mutex_lock(3)");
		x++;
		ret = pthread_mutex_unlock(&mutex);
		if (ret != 0)
			err(1, "pthread_mutex_unlock(3)");
	}

	return (void *)count;
}
