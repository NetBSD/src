/*	$NetBSD: once3.c,v 1.2 2005/07/16 23:12:02 nathanw Exp $	*/

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void ofunc(void);
void* threadfunc(void *);
void cleanup(void *);
void handler(int, siginfo_t *, void *);

int
main(int argc, char *argv[])
{
	pthread_t thread;
	struct sigaction act;
	struct itimerval it;
	printf("Test 3 of pthread_once() (test versus cancellation)\n");

	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGALRM, &act, NULL);

	timerclear(&it.it_value);
	it.it_value.tv_usec = 500000;
	timerclear(&it.it_interval);
	setitimer(ITIMER_REAL, &it, NULL);

	pthread_mutex_lock(&mutex);
	pthread_create(&thread, NULL, threadfunc, NULL);
	pthread_cancel(thread);
	pthread_mutex_unlock(&mutex);
	pthread_join(thread, NULL);

	pthread_once(&once, ofunc);

	/* Cancel timer */
	timerclear(&it.it_value);
	setitimer(ITIMER_REAL, &it, NULL);

	printf("Test succeeded\n");

	return 0;
}

void
cleanup(void *m)
{
	pthread_mutex_t *mu = m;

	pthread_mutex_unlock(mu);
}

void
ofunc(void)
{
	
	pthread_testcancel();
}

void *
threadfunc(void *arg)
{

	pthread_mutex_lock(&mutex);
	pthread_cleanup_push(cleanup, &mutex);
	pthread_once(&once, ofunc);
	pthread_cleanup_pop(1);
	return NULL;
}


void
handler(int sig, siginfo_t *info, void *ctx)
{
	printf("Signal handler was called; main thread deadlocked in "
	    "pthread_once().\n"
	    "Test failed.\n");
	exit(1);
}
