/* $NetBSD: context.c,v 1.3 2004/07/19 06:36:27 chs Exp $ */

#include <malloc.h>
#include <ucontext.h>
#include <stdarg.h>
#include <err.h>

#define STACKSZ (10*1024)
#define DEPTH 3

int calls;

void
run(int n, ...)
{
	va_list va;
	int i, ia;

	if (n != DEPTH - calls - 1)
		exit (1);

	va_start(va, n);
	for (i = 0; i < 9; i++) {
		ia = va_arg(va, int);
		if (ia != i)
			errx(2, "arg[%d]=%d", i, ia);
	}
	va_end(va);
	calls++;
}

main()
{
	ucontext_t uc[DEPTH];
	int i, res;

	for (i = 0; i < DEPTH; i++) {
		res = getcontext(&uc[i]);
		if (res)
			err(1, "getcontext");
		uc[i].uc_stack.ss_sp = malloc(STACKSZ);
		uc[i].uc_stack.ss_size = STACKSZ;
		if (i > 0)
			uc[i].uc_link = &uc[i - 1];
		makecontext(&uc[i], (void *)run, 10, i,
			0, 1, 2, 3, 4, 5, 6, 7, 8);
	}
	res = setcontext(&uc[DEPTH-1]);
	if (res)
		err(1, "setcontext");
	/* NOTREACHED */
	return (3);
}
