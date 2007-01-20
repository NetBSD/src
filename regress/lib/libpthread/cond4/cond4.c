/*	$NetBSD: cond4.c,v 1.2 2007/01/20 19:21:18 christos Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int count, total, toggle;
#define COUNT 50000

int main(int argc, char *argv[])
{
	int ret;
	pthread_t new;
	void *joinval;
	int sharedval;

	printf("1: condition variable test 4\n");

	ret = pthread_mutex_lock(&mutex);
	if (ret)
		err(1, "pthread_mutex_lock(1)");

	count = COUNT;
	toggle = 0;

	ret = pthread_create(&new, NULL, threadfunc, &sharedval);
	if (ret != 0)
		err(1, "pthread_create");

	printf("1: Before waiting.\n");
	while (count>0) {
		count--;
		total++;
		toggle = 1;
#if 0
		printf("1: Before signal %d.\n", count);
#endif
		pthread_cond_signal(&cond);
		do {
			pthread_cond_wait(&cond, &mutex);
		} while (toggle != 0);
	}
	printf("1: After the loop.\n");

	toggle = 1;
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
	
	printf("1: After releasing the mutex.\n");
	ret = pthread_join(new, &joinval);
	if (ret != 0)
		err(1, "pthread_join");

	printf("1: Thread joined. Final count = %d, total = %d\n",
	    count, total);
	assert(count == 0);
	assert(total == COUNT);

	return 0;
}

void *threadfunc(void *arg)
{
#if 0
	int *share = (int *) arg;
#endif

	printf("2: Second thread.\n");
	pthread_mutex_lock(&mutex);
	printf("2: Before the loop.\n");
	while (count>0) {
		count--;
		total++;
		toggle = 0;
#if 0
		printf("2: Before signal %d.\n", count);
#endif
		pthread_cond_signal(&cond);
		do {
			pthread_cond_wait(&cond, &mutex);
		} while (toggle != 1);
	}
	printf("2: After the loop.\n");
	pthread_mutex_unlock(&mutex);

	return NULL;
}
