/*	$NetBSD: _lwp.c,v 1.1.2.1 2001/11/21 20:14:26 scw Exp $	*/

/* Copyright */

#include <sys/types.h>
#include <ucontext.h>
#include <lwp.h>
#include <stdlib.h>



void _lwp_makecontext(ucontext_t *u, void (*start)(void *),
    void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	void **sp;

	getcontext(u);
	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;

	u->uc_mcontext.__gregs[_REG_PC] = (int)start;
	
	sp = (void **) (stack_base + stack_size);
	
	*--sp = arg;
	*--sp = (void *) _lwp_exit;

	u->uc_mcontext.__gregs[_REG_A7] = (int) sp;
}
