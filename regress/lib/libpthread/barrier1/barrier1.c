/*	$NetBSD: barrier1.c,v 1.1 2003/01/30 18:23:09 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadfunc(void *arg);

pthread_barrier_t barrier;
pthread_mutex_t mutex;
int serial_count;
int after_barrier_count;

#define	COUNT	10

int
main(int argc, char *argv[])
{
	int i, ret;
	pthread_t new[COUNT];
	void *joinval;

	pthread_mutex_init(&mutex, NULL);

	ret = pthread_barrier_init(&barrier, NULL, COUNT);
	if (ret != 0)
		err(1, "pthread_barrier_init");

	for (i = 0; i < COUNT; i++) {
		ret = pthread_create(&new[i], NULL, threadfunc,
		    (void *)(long)i);
		if (ret != 0)
			err(1, "pthread_create");
		sleep(2);
	}

	for (i = 0; i < COUNT; i++) {
		pthread_mutex_lock(&mutex);
		assert(after_barrier_count >= (COUNT - i));
		pthread_mutex_unlock(&mutex);
		ret = pthread_join(new[i], &joinval);
		if (ret != 0)
			err(1, "pthread_join");
		printf("main joined with thread %d\n", i);
	}

	assert(serial_count == 1);

	return 0;
}

void *
threadfunc(void *arg)
{
	int which = (int)(long)arg;
	int ret;

	printf("thread %d entering barrier\n", which);
	ret = pthread_barrier_wait(&barrier);
	printf("thread %d leaving barrier -> %d\n", which, ret);
	
	pthread_mutex_lock(&mutex);
	after_barrier_count++;
	if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
		serial_count++;
	pthread_mutex_unlock(&mutex);

	return NULL;
}
