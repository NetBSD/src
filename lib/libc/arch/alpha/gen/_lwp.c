/*	$Id: _lwp.c,v 1.1.2.3 2002/01/28 20:09:50 nathanw Exp $ */

/* Copyright */

#include <sys/types.h>
#include <ucontext.h>
#include <machine/reg.h>
#include <lwp.h>
#include <stdlib.h>



void _lwp_makecontext(ucontext_t *u, void (*start)(void *),
    void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	__greg_t *gr;

	getcontext(u);
	gr = u->uc_mcontext.__gregs;

	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;

	gr[_REG_PC] = (unsigned long)start;
	gr[_REG_T12] = (unsigned long) start;
	gr[_REG_RA] = (unsigned long) _lwp_exit;
	gr[_REG_A0] = (unsigned long) arg;
	gr[_REG_SP] = ((unsigned long) (stack_base + stack_size)) & ~0x7;
	gr[_REG_S6] = 0;
}
