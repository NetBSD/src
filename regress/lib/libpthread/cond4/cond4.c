/*	$NetBSD: cond4.c,v 1.1 2003/01/30 18:53:48 thorpej Exp $	*/

#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int count, total, toggle;

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

	count = 5000000;
	toggle = 0;

	ret = pthread_create(&new, NULL, threadfunc, &sharedval);
	if (ret != 0)
		err(1, "pthread_create");

	printf("1: Before waiting.\n");
	while (count>0) {
		count--;
		total++;
		toggle = 1;
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
	assert(total == 5000000);

	return 0;
}

void *threadfunc(void *arg)
{
#if 0
	int *share = (int *) arg;
#endif

	printf("2: Second thread.\n");
	pthread_mutex_lock(&mutex);
	while (count>0) {
		count--;
		total++;
		toggle = 0;
		pthread_cond_signal(&cond);
		do {
			pthread_cond_wait(&cond, &mutex);
		} while (toggle != 1);
	}
	printf("2: After the loop.\n");
	pthread_mutex_unlock(&mutex);

	return NULL;
}
