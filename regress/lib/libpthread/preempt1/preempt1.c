/*	$NetBSD: preempt1.c,v 1.1 2003/01/31 20:14:25 skrll Exp $	*/

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadfunc(void *arg);

pthread_mutex_t mutex;
pthread_cond_t cond;
int started;

#define HUGE_BUFFER	1<<20
#define NTHREADS	1

int
main(int argc, char *argv[])
{
	int ret, i;
	pthread_t new;
	void *joinval;

	char *mem;
	int fd;

	mem = malloc(HUGE_BUFFER);
	if (mem == NULL)
		err(1, "malloc");

	fd = open("/dev/urandom", O_RDONLY, 0);
	if (fd == 0)
		err(1, "open");

	printf("1: preempt test\n");

	pthread_cond_init(&cond, NULL);
	pthread_mutex_lock(&mutex);

	started = 0;

	for (i = 0; i < NTHREADS; i++) {
		ret = pthread_create(&new, NULL, threadfunc, NULL);
		if (ret != 0)
			err(1, "pthread_create");
	}

	while (started < NTHREADS) {
		pthread_cond_wait(&cond, &mutex);
	}

	printf("1: Thread has started.\n");

	pthread_mutex_unlock(&mutex);
	printf("1: After releasing the mutex.\n");

	ret = read(fd, mem, HUGE_BUFFER);
	close(fd);

	assert(ret == HUGE_BUFFER);

	ret = pthread_join(new, &joinval);
	if (ret != 0)
		err(1, "pthread_join");

	printf("1: Thread joined.\n");

	return 0;
}

void *
threadfunc(void *arg)
{
	printf("2: Second thread.\n");

	printf("2: Locking mutex\n");
	pthread_mutex_lock(&mutex);
	printf("2: Got mutex.\n");
	started++;
	
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
	sleep(1);

	return NULL;
}
