/* $NetBSD: tramptest.c,v 1.2 2004/02/19 16:49:43 drochner Exp $ */

#include <stdlib.h>
#include <signal.h>

/*
 * This test checks that the stack has no execute permission.
 * It depends on the fact that gcc puts trampoline code for
 * nested functions on the stack, at least on some architectures.
 * (On the other architectures, the test will fail, as on platforms
 * where execution permissions cannot be controlled.)
 * Actually, it would be better if gcc wouldn't use stack trampolines,
 * at all, but for now it allows for an easy portable check whether the
 * stack is executable.
 */

void
__enable_execute_stack()
{
	/* replace gcc's internal function by a noop */
}

void
buserr(int s, siginfo_t *si, void *ctx)
{

	if (s != SIGSEGV || si->si_code != SEGV_ACCERR)
		exit(2);

	exit(0);
}

void (*f)(void);

void do_f()
{

	(*f)();
}

int
main()
{
	struct sigaction sa;

	void mist()
	{

		return;
	}

	sa.sa_sigaction = buserr;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, 0);

	f = mist;
	do_f();

	exit(1);
}
