/* $NetBSD: tramptest.c,v 1.1 2003/12/10 13:24:59 drochner Exp $ */

#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

/*
 * This test checks whether processes/threads get execute permission
 * on the stack if needed, in particular for multiple threads.
 * It depends on the fact that gcc puts trampoline code for
 * nested functions on the stack and requests execution permission
 * for that address internally, at least on some architectures.
 * (On the other architectures, the test is just insignificant.)
 * Actually, it would be better if gcc wouldn't use stack trampolines,
 * at all, but for now it allows for an easy portable check whether the
 * kernel handles permissions correctly.
 */

void
buserr(int s)
{

	exit(1);
}

int
main()
{
	pthread_t t1, t2;

	void *mist(void *p)
	{

		return (0);
	}

	signal(SIGBUS, buserr);

	pthread_create(&t1, 0, mist, 0);
	pthread_create(&t2, 0, mist, 0);
	pthread_join(t1, 0);
	pthread_join(t2, 0);
	exit(0);
}
