/*	$NetBSD: arm_machdep.c,v 1.2.6.3 2001/11/16 16:58:37 thorpej Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_compat_netbsd.h"
#include "opt_progmode.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: arm_machdep.c,v 1.2.6.3 2001/11/16 16:58:37 thorpej Exp $");

#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/pool.h>
#include <sys/lwp.h>
#include <sys/ucontext.h>
#include <sys/savar.h>

#include <machine/pcb.h>
#include <machine/cpufunc.h>
#include <machine/vmparam.h>

static __inline struct trapframe *
process_frame(struct lwp *l)
{

	return (l->l_addr->u_pcb.pcb_tf);
}

/*
 * Clear registers on exec
 */

void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct trapframe *tf;

	tf = l->l_addr->u_pcb.pcb_tf;

	memset(tf, 0, sizeof(*tf));
	tf->tf_r0 = (u_int)PS_STRINGS;
#ifdef COMPAT_13
	tf->tf_r12 = stack;			/* needed by pre 1.4 crt0.c */
#endif
	tf->tf_usr_sp = stack;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_pc = pack->ep_entry;
#ifdef PROG32
	tf->tf_spsr = PSR_USR32_MODE;
#endif

	l->l_addr->u_pcb.pcb_flags = 0;
}

/*
 * startlwp:
 *
 *	Start a new LWP.
 */
void
startlwp(void *arg)
{
	int err;
	ucontext_t *uc = arg; 
	struct lwp *l = curproc;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#ifdef DIAGNOSTIC
	if (err)
		printf("Error %d from cpu_setmcontext.", err);
#endif
	pool_put(&lwp_uc_pool, uc);

	userret(l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(void *arg)
{
	struct lwp *l = curproc;

	userret(l);
}

/*
 * cpu_stashcontext:
 *
 *	Save the user-level ucontext_t on the LWP's own stack.
 */
ucontext_t *
cpu_stashcontext(struct lwp *l)
{
	struct trapframe *tf;
	ucontext_t u, *up;
	void *stack;

	tf = process_frame(l);

	stack = (void *)(tf->tf_usr_sp - sizeof(ucontext_t));

	getucontext(l, &u);
	up = stack;

	if (copyout(&u, stack, sizeof(ucontext_t)) != 0) {
		/* Copying onto the stack didn't work.  Die. */
#ifdef DIAGNOSTIC
		printf("cpu_stashcontext: couldn't copyout context of %d.%d\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	return (up);
}

/*
 * cpu_upcall:
 *
 *	Send an an upcall to userland.
 */
void
cpu_upcall(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sd = p->p_sa;
	struct saframe *sf, frame;
	struct sa_t **sapp, *sap;
	struct sa_t self_sa, e_sa, int_sa;
	struct sa_t *sas[3];
	struct sadata_upcall *sau;
	struct trapframe *tf;
	void *stack;
	ucontext_t u, *up;
	int i, nsas, nevents, nint;
	int x, y;

	extern char sigcode[], upcallcode[];

	tf = process_frame(l);

	KDASSERT(LIST_EMPTY(&sd->sa_upcalls) == 0);

	sau = LIST_FIRST(&sd->sa_upcalls);

	stack = (char *)sau->sau_stack.ss_sp + sau->sau_stack.ss_size;

	self_sa.sa_id = l->l_lid;
	self_sa.sa_cpu = 0; /* XXX l->l_cpu; */
	sas[0] = &self_sa;
	nsas = 1;

	nevents = 0;
	if (sau->sau_event) {
		e_sa.sa_context = cpu_stashcontext(sau->sau_event);
		e_sa.sa_id = sau->sau_event->l_lid;
		e_sa.sa_cpu = 0; /* XXX event->l_cpu; */
		sas[nsas++] = &e_sa;
		nevents = 1;
	}

	nint = 0;
	if (sau->sau_interrupted) {
		int_sa.sa_context = cpu_stashcontext(sau->sau_interrupted);
		int_sa.sa_id = sau->sau_interrupted->l_lid;
		int_sa.sa_cpu = 0; /* XXX interrupted->l_cpu; */
		sas[nsas++] = &int_sa;
		nint = 1;
	}

	LIST_REMOVE(sau, sau_next);
	if (LIST_EMPTY(&sd->sa_upcalls))
		l->l_flag &= ~L_SA_UPCALL;

	/* Copy out the activation's ucontext */
	u.uc_stack = sau->sau_stack;
	u.uc_flags = _UC_STACK;
	up = stack;
	up--;
	if (copyout(&u, up, sizeof(ucontext_t)) != 0) {
		pool_put(&saupcall_pool, sau);
#ifdef DIAGNOSTIC
		printf("cpu_upcall: couldn't copyout activation ucontext"
		    " for %d.%d\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
	sas[0]->sa_context = up;

	/* Next, copy out the sa_t's and pointers to them. */
	sap = (struct sa_t *) up;
	sapp = (struct sa_t **) (sap - nsas);
	for (i = nsas - 1; i >= 0; i--) {
		sap--;
		sapp--;
		if (((x=copyout(sas[i], sap, sizeof(struct sa_t)) != 0)) ||
		    ((y=copyout(&sap, sapp, sizeof(struct sa_t *)) != 0))) {
			/* Copy onto the stack didn't work.  Die. */
			pool_put(&saupcall_pool, sau);
#ifdef DIAGNOSTIC
			printf("cpu_upcall: couldn't copyout sa_t %d"
			    " for %d.%d (x=%d, y=%d)\n",
			    i, l->l_proc->p_pid, l->l_lid, x, y);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	}

	/* Finally, copy out the rest of the frame. */
	sf = (struct saframe *)sapp - 1;

#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = sau->sau_type;
	frame.sa_sas = sapp;
	frame.sa_events = nevents;
	frame.sa_interrupted = nint;
#endif
	frame.sa_sig = sau->sau_sig;
	frame.sa_code = sau->sau_code;
	frame.sa_arg = sau->sau_arg;
	frame.sa_upcall = sd->sa_upcall;

	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work. Die. */
		pool_put(&saupcall_pool, sau);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_r0 = sau->sau_type;
	tf->tf_r1 = (int) sapp;
	tf->tf_r2 = nevents;
	tf->tf_r3 = nint;
	tf->tf_usr_sp = (int) sf;
	tf->tf_pc = (int) ((caddr_t)p->p_sigctx.ps_sigcode + (
	    (caddr_t)upcallcode - (caddr_t)sigcode));
#ifndef arm26
	cpu_cache_syncI();	/* XXX really necessary? */
#endif

	pool_put(&saupcall_pool, sau);
}
