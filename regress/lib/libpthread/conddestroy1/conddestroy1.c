/*	$NetBSD: conddestroy1.c,v 1.1 2004/07/07 21:53:10 nathanw Exp $ */

#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void handler(int);
void *threadroutine(void *);

pthread_mutex_t mt;
pthread_cond_t cv;

void
handler(int sig)
{
	/* Dummy */
	return;
}

void *
threadroutine(void *arg)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);

	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	pthread_mutex_lock(&mt);

	/*
	 * Explicitly not a loop; we want to see if the cv is properly 
	 * torn down in a spurious wakeup (generated here by SIGALRM).
	 */
	pthread_cond_wait(&cv, &mt);

	pthread_mutex_unlock(&mt);

	return NULL;
}

int
main(void)
{
	pthread_t th;
	sigset_t set;
	struct sigaction act;

	printf("Testing CV teardown under spurious wakeups.\n");

	sigfillset(&set);

	pthread_sigmask(SIG_BLOCK, &set, NULL);

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGALRM, &act, NULL);

	pthread_mutex_init(&mt, NULL);
	pthread_cond_init(&cv, NULL);

	pthread_create(&th, NULL, threadroutine, NULL);
	
	alarm(1);

	pthread_join(th, NULL);

	pthread_cond_destroy(&cv);
	pthread_mutex_destroy(&mt);

	printf("Passed.\n");
	return 0;
}
