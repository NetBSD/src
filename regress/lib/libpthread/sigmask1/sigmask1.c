/*	$NetBSD: sigmask1.c,v 1.1 2004/01/02 19:26:24 cl Exp $	*/

/*
 * Regression test for pthread_sigmask when SA upcalls aren't started yet.
 *
 * Written by Christian Limpach <cl@NetBSD.org>, December 2003.
 * Public domain.
 */

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <sys/resource.h>

int
main(int argc, char **argv)
{
	sigset_t nset;
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

	sigemptyset(&nset);
	sigaddset(&nset, SIGFPE);
	pthread_sigmask(SIG_BLOCK, &nset, NULL);

	kill(getpid(), SIGFPE);

	return 0;
}

