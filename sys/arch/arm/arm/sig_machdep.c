/*	$NetBSD: sig_machdep.c,v 1.7.6.6 2002/02/28 04:07:18 nathanw Exp $	*/

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
 *
 * Machine dependant functions for kernel setup
 *
 * Created      : 17/09/94
 */

#include "opt_compat_netbsd.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.7.6.6 2002/02/28 04:07:18 nathanw Exp $");

#include <sys/mount.h>		/* XXX only needed by syscallargs.h */
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/lwp.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <arm/armreg.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#ifndef arm26
#include <arm/cpufunc.h>
#endif

static __inline struct trapframe *
process_frame(struct lwp *l)
{

	return l->l_addr->u_pcb.pcb_tf;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user specified pc.
 */
void
sendsig(sig_t catcher, int sig, sigset_t *mask, u_long code)
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	int onstack;

	tf = process_frame(l);

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)tf->tf_usr_sp;
	fp--;
	(u_int)fp = STACKALIGN(fp);

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;
	frame.sf_handler = catcher;

	/* Save register context. */
	frame.sf_sc.sc_r0     = tf->tf_r0;
	frame.sf_sc.sc_r1     = tf->tf_r1;
	frame.sf_sc.sc_r2     = tf->tf_r2;
	frame.sf_sc.sc_r3     = tf->tf_r3;
	frame.sf_sc.sc_r4     = tf->tf_r4;
	frame.sf_sc.sc_r5     = tf->tf_r5;
	frame.sf_sc.sc_r6     = tf->tf_r6;
	frame.sf_sc.sc_r7     = tf->tf_r7;
	frame.sf_sc.sc_r8     = tf->tf_r8;
	frame.sf_sc.sc_r9     = tf->tf_r9;
	frame.sf_sc.sc_r10    = tf->tf_r10;
	frame.sf_sc.sc_r11    = tf->tf_r11;
	frame.sf_sc.sc_r12    = tf->tf_r12;
	frame.sf_sc.sc_usr_sp = tf->tf_usr_sp;
	frame.sf_sc.sc_usr_lr = tf->tf_usr_lr;
	frame.sf_sc.sc_svc_lr = tf->tf_svc_lr;
	frame.sf_sc.sc_pc     = tf->tf_pc;
	frame.sf_sc.sc_spsr   = tf->tf_spsr;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_r0 = frame.sf_signum;
	tf->tf_r1 = frame.sf_code;
	tf->tf_r2 = (int)frame.sf_scp;
	tf->tf_r3 = (int)frame.sf_handler;
	tf->tf_usr_sp = (int)fp;
	tf->tf_pc = (int)p->p_sigctx.ps_sigcode;
#ifndef arm26
	/* XXX This should not be needed. */
	cpu_icache_sync_all();
#endif

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}


/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psr to gain improper privileges or to cause
 * a machine fault.
 */
int
sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/*
	 * Make sure the processor mode has not been tampered with and
	 * interrupts have not been disabled.
	 */
#ifdef __PROG32
	if ((context.sc_spsr & PSR_MODE) != PSR_USR32_MODE ||
	    (context.sc_spsr & (I32_bit | F32_bit)) != 0)
		return (EINVAL);
#else /* __PROG26 */
	if ((context.sc_pc & R15_MODE) != R15_MODE_USR ||
	    (context.sc_pc & (R15_IRQ_DISABLE | R15_FIQ_DISABLE)) != 0)
		return EINVAL;
#endif

	/* Restore register context. */
	tf = process_frame(l);
	tf->tf_r0    = context.sc_r0;
	tf->tf_r1    = context.sc_r1;
	tf->tf_r2    = context.sc_r2;
	tf->tf_r3    = context.sc_r3;
	tf->tf_r4    = context.sc_r4;
	tf->tf_r5    = context.sc_r5;
	tf->tf_r6    = context.sc_r6;
	tf->tf_r7    = context.sc_r7;
	tf->tf_r8    = context.sc_r8;
	tf->tf_r9    = context.sc_r9;
	tf->tf_r10   = context.sc_r10;
	tf->tf_r11   = context.sc_r11;
	tf->tf_r12   = context.sc_r12;
	tf->tf_usr_sp = context.sc_usr_sp;
	tf->tf_usr_lr = context.sc_usr_lr;
	tf->tf_svc_lr = context.sc_svc_lr;
	tf->tf_pc    = context.sc_pc;
	tf->tf_spsr  = context.sc_spsr;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}

void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	struct trapframe *tf = process_frame(l);
	__greg_t *gr = mcp->__gregs;
	
	/* Save General Register context. */
	gr[_REG_R0]   = tf->tf_r0;
	gr[_REG_R1]   = tf->tf_r1;
	gr[_REG_R2]   = tf->tf_r2;
	gr[_REG_R3]   = tf->tf_r3;
	gr[_REG_R4]   = tf->tf_r4;
	gr[_REG_R5]   = tf->tf_r5;
	gr[_REG_R6]   = tf->tf_r6;
	gr[_REG_R7]   = tf->tf_r7;
	gr[_REG_R8]   = tf->tf_r8;
	gr[_REG_R9]   = tf->tf_r9;
	gr[_REG_R10]  = tf->tf_r10;
	gr[_REG_R11]  = tf->tf_r11;
	gr[_REG_R12]  = tf->tf_r12;
	gr[_REG_SP]   = tf->tf_usr_sp;
	gr[_REG_LR]   = tf->tf_usr_lr;
	gr[_REG_PC]   = tf->tf_pc;
	gr[_REG_CPSR] = tf->tf_spsr;
	*flags |= _UC_CPU;

#ifdef ARMFPE
	/* Save Floating Point Register context. */
	arm_fpe_getcontext(p, (struct fpreg *)(void *)&mcp->fpregs);
	*flags |= _UC_FPU;
#endif
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf = process_frame(l);
	__greg_t *gr = mcp->__gregs;

	if ((flags & _UC_CPU) != 0) {
		/* Restore General Register context. */
		/* Make sure the processor mode has not been tampered with. */
#ifdef PROG32
		if ((gr[_REG_CPSR] & PSR_MODE) != PSR_USR32_MODE ||
		    (gr[_REG_CPSR] & (I32_bit | F32_bit)) != 0)
			return (EINVAL);
#else /* PROG26 */
		if ((gr[_REG_PC] & R15_MODE) != R15_MODE_USR ||
		    (gr[_REG_PC] & (R15_IRQ_DISABLE | R15_FIQ_DISABLE)) != 0)
			return (EINVAL);
#endif

		tf->tf_r0     = gr[_REG_R0];
		tf->tf_r1     = gr[_REG_R1];
		tf->tf_r2     = gr[_REG_R2];
		tf->tf_r3     = gr[_REG_R3];
		tf->tf_r4     = gr[_REG_R4];
		tf->tf_r5     = gr[_REG_R5];
		tf->tf_r6     = gr[_REG_R6];
		tf->tf_r7     = gr[_REG_R7];
		tf->tf_r8     = gr[_REG_R8];
		tf->tf_r9     = gr[_REG_R9];
		tf->tf_r10    = gr[_REG_R10];
		tf->tf_r11    = gr[_REG_R11];
		tf->tf_r12    = gr[_REG_R12];
		tf->tf_usr_sp = gr[_REG_SP];
		tf->tf_usr_lr = gr[_REG_LR];
		tf->tf_pc     = gr[_REG_PC];
		tf->tf_spsr   = gr[_REG_CPSR];
	}

#ifdef ARMFPE
	if ((flags & _UC_FPU) != 0) {
		/* Restore Floating Point Register context. */
		arm_fpe_setcontext(p, (struct fpreg *)(void *)&mcp->__fpregs);
	}
#endif

	return (0);
}
