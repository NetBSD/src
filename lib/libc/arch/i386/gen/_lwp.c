/*	$Id: _lwp.c,v 1.1.2.2 2001/12/18 21:25:24 nathanw Exp $ */

/* Copyright */

#include <inttypes.h>
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

	/* LINTED uintptr_t is safe */
	u->uc_mcontext.__gregs[_REG_EIP] = (uintptr_t)start;
	
	/* Align to a word */
	sp = (void **) ((uintptr_t)(stack_base + stack_size) & ~0x3);
	
	*--sp = arg;
	*--sp = (void *) _lwp_exit;
	
	/* LINTED uintptr_t is safe */
	u->uc_mcontext.__gregs[_REG_UESP] = (uintptr_t) sp;

	/* LINTED private is currently unused */
}
