/*	$NetBSD: _lwp.c,v 1.1.2.3 2002/12/18 17:39:59 skrll Exp $ */

#include <sys/types.h>
#include <ucontext.h>
#include <lwp.h>
#include <stdlib.h>


void
_lwp_makecontext(ucontext_t *u, void (*start)(void *), void *arg,
		 void *private, caddr_t stack_base, size_t stack_size)
{
	__greg_t *gr;
	unsigned long *sp;

	getcontext(u);
	gr = u->uc_mcontext.__gregs;

	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;


	sp = (ulong *)(stack_base + stack_size);
	sp = (ulong *)((ulong)sp & ~0x07);

	/* Make room for the fake caller stack frame (CCFSZ, only in words) */
	sp -= 8 + 8 + 1 + 6 + 1;

	/* Entering (*start)(arg), return is to _lwp_exit */
	gr[_REG_PC] = (ulong) start;
	gr[_REG_nPC] = (ulong) start + 4;
	gr[_REG_O0] = (ulong)arg;
	gr[_REG_O6] = (ulong)sp;
	gr[_REG_O7] = (ulong)_lwp_exit - 8;

	/* XXX: uwe: why do we need this? */
	/* create loopback in the window save area on the stack? */
	sp[8+6] = (ulong)sp;		/* %i6 */
	sp[8+7] = (ulong)_lwp_exit - 8;	/* %i7 */
}
