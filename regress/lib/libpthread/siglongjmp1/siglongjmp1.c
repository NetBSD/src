/*	$NetBSD: siglongjmp1.c,v 1.1 2004/01/02 19:27:06 cl Exp $	*/

/*
 * Regression test for siglongjmp out of a signal handler back into
 * its thread.
 *
 * Written by Christian Limpach <cl@NetBSD.org>, December 2003.
 * Public domain.
 */

#include <err.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#include <sys/resource.h>

void *thread(void *);
void handler(int, siginfo_t *, void *);
static sigjmp_buf env;

void *
thread(void *arg)
{
	return NULL;
}

void
handler(int sig, siginfo_t *info, void *ctx)
{
	siglongjmp(env, 1);
}

int
main(int argc, char **argv)
{
	pthread_t t;
	sigset_t nset;
	struct rlimit rlim;
	struct sigaction act;

	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

	pthread_create(&t, NULL, thread, NULL);

	sigemptyset(&nset);
	sigaddset(&nset, SIGUSR1);
	pthread_sigmask(SIG_SETMASK, &nset, NULL);

	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR2);
	act.sa_flags = 0;
	sigaction(SIGSEGV, &act, NULL);

	if (sigsetjmp(env, 1) == 0)
		*(long *)0 = 0;

	pthread_sigmask(0, NULL, &nset);

	if (sigismember(&nset, SIGSEGV))
		errx(1, "SIGSEGV set");
	if (sigismember(&nset, SIGUSR2))
		errx(1, "SIGUSR2 set");
	if (!sigismember(&nset, SIGUSR1))
		errx(1, "SIGUSR1 not set");

	return 0;
}

