/*	$Id: _lwp.c,v 1.1.2.1 2001/11/17 03:19:29 nathanw Exp $ */

/* Copyright */

#include <sys/types.h>
#include <ucontext.h>
#include <machine/reg.h>
#include <lwp.h>
#include <stdlib.h>



void _lwp_makecontext(ucontext_t *u, void (*start)(void *),
    void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	unsigned long *gr = u->uc_mcontext.sc_regs;

	getcontext(u);
	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;

	u->uc_mcontext.sc_pc = (unsigned long)start;
	gr[R_T12] = (unsigned long) start;
	gr[R_RA] = (unsigned long) _lwp_exit;
	gr[R_A0] = (unsigned long) arg;
	gr[R_SP] = (unsigned long) (stack_base + stack_size);
	gr[R_S6] = 0;
}
