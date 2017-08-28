/*	$NetBSD: compat_16_machdep.c,v 1.21.6.1 2017/08/28 17:51:45 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris Demetriou.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
	
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.21.6.1 2017/08/28 17:51:45 skrll Exp $"); 

#ifdef _KERNEL_OPT
#include "opt_cputype.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <mips/regnum.h>
#include <mips/frame.h>
#include <mips/locore.h>
#include <mips/pcb.h>
#include <mips/reg.h>

#if !defined(__mips_o32)
#define	fpreg		fpreg_oabi
#endif

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send a signal to process.
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *returnmask)
{
	int sig = ksi->ksi_signo;
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct sigacts * const ps = p->p_sigacts;
	struct pcb * const pcb = lwp_getpcb(l);
	int onstack, error;
	struct sigcontext *scp = getframe(l, sig, &onstack);
	struct sigcontext ksc;
	struct trapframe * const tf = l->l_md.md_utf;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

#if !defined(__mips_o32)
	if (p->p_md.md_abi != _MIPS_BSD_API_O32)
		sigexit(l, SIGILL);
#endif

	scp--;

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d ssp %p scp %p\n",
		       p->p_pid, sig, &onstack, scp);
#endif

	/* Build stack frame for signal trampoline. */
	ksc.sc_pc = tf->tf_regs[_R_PC];
	ksc.mullo = tf->tf_regs[_R_MULLO];
	ksc.mulhi = tf->tf_regs[_R_MULHI];

	/* Save register context. */
	ksc.sc_regs[_R_ZERO] = 0xACEDBADE;		/* magic number */
#if defined(__mips_o32)
	memcpy(&ksc.sc_regs[1], &tf->tf_regs[1],
	    sizeof(ksc.sc_regs) - sizeof(ksc.sc_regs[0]));
#else
	for (size_t i = 1; i < 32; i++)
		ksc.sc_regs[i] = tf->tf_regs[i];
#endif

	/* Save the FP state, if necessary, then copy it. */
	ksc.sc_fpused = fpu_used_p(l);
#if !defined(NOFPU)
	if (ksc.sc_fpused) {
		/* if FPU has current state, save it first */
		fpu_save(l);
	}
#endif
	*(struct fpreg *)ksc.sc_fpregs = *(struct fpreg *)&pcb->pcb_fpregs;

	/* Save signal stack. */
	ksc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	ksc.sc_mask = *returnmask;

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(returnmask, &ksc.__sc_mask13);
#endif

	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	error = copyout(&ksc, (void *)scp, sizeof(ksc));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_FOLLOW) ||
		    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
			printf("sendsig(%d): copyout failed on sig %d\n",
			    p->p_pid, sig);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Set up the registers to directly invoke the signal
	 * handler.  The return address will be set up to point
	 * to the signal trampoline to bounce us back.
	 */
	tf->tf_regs[_R_A0] = sig;
	tf->tf_regs[_R_A1] = ksi->ksi_trap;
	tf->tf_regs[_R_A2] = (intptr_t)scp;
	tf->tf_regs[_R_A3] = (intptr_t)catcher;		/* XXX ??? */

	tf->tf_regs[_R_PC] = (intptr_t)catcher;
	tf->tf_regs[_R_T9] = (intptr_t)catcher;
	tf->tf_regs[_R_SP] = (intptr_t)scp;

	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		tf->tf_regs[_R_RA] = (intptr_t)p->p_sigctx.ps_sigcode;
		break;
#ifdef COMPAT_16
	case 1:
		tf->tf_regs[_R_RA] = (intptr_t)ps->sa_sigdesc[sig].sd_tramp;
		break;
#endif
	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}

#ifdef COMPAT_16 /* not needed if COMPAT_ULTRIX only */
/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
compat_16_sys___sigreturn14(struct lwp *l, const struct compat_16_sys___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct sigcontext *scp, ksc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
	struct pcb * const pcb = lwp_getpcb(l);
	int error;

#if !defined(__mips_o32)
	if (p->p_md.md_abi != _MIPS_BSD_API_O32)
		return ENOSYS;
#endif

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", l->l_proc->p_pid, scp);
#endif
	if ((error = copyin(scp, &ksc, sizeof(ksc))) != 0)
		return (error);

	if ((u_int) ksc.sc_regs[_R_ZERO] != 0xacedbadeU)/* magic number */
		return (EINVAL);

	/* Restore the register context. */
	tf->tf_regs[_R_PC] = ksc.sc_pc;
	tf->tf_regs[_R_MULLO] = ksc.mullo;
	tf->tf_regs[_R_MULHI] = ksc.mulhi;
#if defined(__mips_o32)
	memcpy(&tf->tf_regs[1], &scp->sc_regs[1],
	    sizeof(scp->sc_regs) - sizeof(scp->sc_regs[0]));
#else
	for (size_t i = 1; i < __arraycount(ksc.sc_regs); i++)
		tf->tf_regs[i] = ksc.sc_regs[i];
#endif
#if !defined(NOFPU)
	if (scp->sc_fpused) {
		fpu_discard(l);
	}
#endif
	*(struct fpreg *)&pcb->pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;

	mutex_enter(p->p_lock);
	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &ksc.sc_mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */
