/* $NetBSD: tramptest.c,v 1.1 2003/12/10 13:24:59 drochner Exp $ */

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
buserr(int s)
{

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

	void mist()
	{

		return;
	}

	signal(SIGBUS, buserr);

	f = mist;
	do_f();

	exit(1);
}
