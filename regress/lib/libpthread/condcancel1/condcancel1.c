/*	$Id: condcancel1.c,v 1.2 2003/11/21 19:25:50 nathanw Exp $	*/

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

void *threadfunc(void *);
void unlock(void *);

pthread_mutex_t mutex;
pthread_cond_t cond;
int share;

int
main(int argc, char *argv[])
{
	pthread_t thread;
	int ret;

	printf("Test of CV state after cancelling a wait\n");

	ret = pthread_mutex_init(&mutex, NULL);
	if (ret) errx(1, "pthread_mutex_init: %s", strerror(ret));

	ret = pthread_cond_init(&cond, NULL);
	if (ret) errx(1, "pthread_cond_init: %s", strerror(ret));

	ret = pthread_mutex_lock(&mutex);
	if (ret) errx(1, "pthread_mutex_lock: %s", strerror(ret));
	
	ret = pthread_create(&thread, NULL, threadfunc, NULL);
	if (ret) errx(1, "pthread_create: %s", strerror(ret));

	while (share == 0) {
		ret = pthread_cond_wait(&cond, &mutex);
		if (ret) errx(1, "pthread_cond_wait: %s", strerror(ret));
	}

	ret = pthread_mutex_unlock(&mutex);
	if (ret) errx(1, "pthread_mutex_unlock: %s", strerror(ret));
	
	ret = pthread_cancel(thread);
	if (ret) errx(1, "pthread_cancel: %s", strerror(ret));

	ret = pthread_join(thread, NULL);
	if (ret) errx(1, "pthread_join: %s", strerror(ret));

	ret = pthread_cond_destroy(&cond);
	if (ret) errx(1, "pthread_cond_destroy: %s", strerror(ret));

	printf("CV successfully destroyed.\n");

	ret = pthread_mutex_destroy(&mutex);
	if (ret) errx(1, "pthread_mutex_destroy: %s", strerror(ret));

	return 0;
}

void *
threadfunc(void *arg)
{
	int ret;

	ret = pthread_mutex_lock(&mutex);
	if (ret) errx(1, "pthread_mutex_lock: %s", strerror(ret));

	pthread_cleanup_push(unlock, &mutex);

	while (1) {
		share = 1;
		ret = pthread_cond_broadcast(&cond);
		if (ret) errx(1, "pthread_cond_broadcast: %s", strerror(ret));
		ret = pthread_cond_wait(&cond, &mutex);
		if (ret) errx(1, "pthread_cond_wait: %s", strerror(ret));
	}

	pthread_cleanup_pop(0);
	ret = pthread_mutex_unlock(&mutex);
	if (ret) errx(1, "pthread_mutex_unlock: %s", strerror(ret));



	return NULL;
}

void
unlock(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}
