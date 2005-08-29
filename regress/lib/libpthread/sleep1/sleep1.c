/*	$NetBSD: sleep1.c,v 1.2 2005/08/29 18:52:16 drochner Exp $	*/

#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/time.h>

void *threadfunc(void *);
void handler(int);

/*
 * More than enough to cross the border from the present time until time_t
 * wraps in 2038.
 */
#define LONGTIME 2000000000 

int
main(void)
{
	pthread_t thread;
	struct itimerval it;
	struct sigaction act;
	sigset_t mtsm;

	printf("Testing sleeps unreasonably far into the future.\n");

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);
	
	pthread_create(&thread, NULL, threadfunc, NULL);

	/* make sure the signal is delivered to the child thread */
	sigemptyset(&mtsm);
	sigaddset(&mtsm, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &mtsm, 0);

	timerclear(&it.it_interval);
	timerclear(&it.it_value);
	it.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &it, NULL);

	pthread_join(thread, NULL);

	printf("Passed.\n");

	return 0;
}

void *
threadfunc(void *arg)
{
	sleep(LONGTIME);
	return NULL;
}

void
handler(int sig)
{
	/*
	 * Nothing to do; invoking the handler is enough to interrupt
	 * the sleep.
	 */
}
