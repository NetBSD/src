/*	$NetBSD: sigalarm.c,v 1.1 2003/09/12 21:15:06 christos Exp $	*/

/*
 * Regression test for sigsuspend in libpthread when pthread lib is
 * initialized.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>


int alarm_set;

#ifdef SA_SIGINFO
static void alarm_handler(int, siginfo_t *, void *);
static void
alarm_handler(int signo, siginfo_t *si, void *ctx)
{
	if (si->si_signo != signo)
		errx(1, "Received unexpected signal %d", signo);
	alarm_set = 1;
}
#else
static void alarm_handler(int);
static void
alarm_handler(int signo)
{
	if (SIGALRM != signo)
		errx(1, "Received unexpected signal %d", signo);
	alarm_set = 1;
}
#endif

static void *
setup(void *dummy)
{
	struct sigaction sa;
#ifdef SA_SIGINFO
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = alarm_handler;
#else
	sa.sa_flags = 0;
	sa.sa_handler = alarm_handler;
#endif
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	alarm(1);
	return NULL;
}

int
main(int argc, char **argv)
{
	sigset_t set;
        pthread_t self = pthread_self();

	if (pthread_create(&self, NULL, setup, NULL) != 0)
		err(1, "Cannot create thread");

	sigemptyset(&set);
	sigsuspend(&set);
	alarm(0);

	if (!alarm_set)
		errx(1, "alarm_set not set");
	return 0;
}
