/*	$NetBSD: _lwp.c,v 1.1.2.1 2002/01/24 02:35:31 petrov Exp $ */

#include <sys/types.h>
#include <ucontext.h>
#include <lwp.h>
#include <stdlib.h>


void _lwp_makecontext(ucontext_t *u, void (*start)(void *),
		      void *arg, void *private,
		      caddr_t stack_base, size_t stack_size)
{
	__greg_t *gr;
	unsigned long *sp;

	getcontext(u);
	gr = u->uc_mcontext.__gregs;

	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;

	sp = (ulong *)(stack_base + stack_size);
	sp = (ulong *)((ulong)sp & ~0x0f);

	sp -= 8 + 8 + 6;

	sp[8]  = (ulong) arg;
	sp[14] = (ulong)((caddr_t)sp - 2047);
	sp[15] = (ulong)(_lwp_exit - 8);

	sp = (ulong *)((caddr_t)sp - 2047);

	gr[_REG_PC] = (ulong) start;
	gr[_REG_nPC] = (ulong) start + 4;

	gr[_REG_O0] = (ulong) arg;
	gr[_REG_O6] = (ulong) sp;
	gr[_REG_O7] = (ulong) _lwp_exit - 8;
}
