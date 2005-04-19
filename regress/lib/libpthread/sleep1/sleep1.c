/*	$NetBSD: sleep1.c,v 1.1 2005/04/19 16:36:34 nathanw Exp $	*/

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

	printf("Testing sleeps unreasonably far into the future.\n");

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);
	
	pthread_create(&thread, NULL, threadfunc, NULL);

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
