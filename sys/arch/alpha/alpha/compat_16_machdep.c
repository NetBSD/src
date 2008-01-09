/* $NetBSD: compat_16_machdep.c,v 1.11.20.1 2008/01/09 01:44:31 matt Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_dec_3000_300.h"
#include "opt_dec_3000_500.h"
#include "opt_compat_osf1.h"
#include "opt_compat_netbsd.h"
#include "opt_execfmt.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/user.h>

#if defined(COMPAT_13) || defined(COMPAT_OSF1)
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#endif

#include <machine/cpu.h>
#include <machine/reg.h>

__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.11.20.1 2008/01/09 01:44:31 matt Exp $");


#ifdef DEBUG
#include <machine/sigdebug.h>
#endif

#include <machine/alpha.h>

#include "ksyms.h"

/*
 * Send an interrupt to process, old style
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, sig = ksi->ksi_signo, error;
	struct sigframe_sigcontext *fp, frame;
	struct trapframe *tf;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = l->l_md.md_tf;
	fp = getframe(l, sig, &onstack), frame;
	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig_sigcontext(%d): sig %d ssp %p usp %p\n",
		       p->p_pid, sig, &onstack, fp);
#endif

	/* Build stack frame for signal trampoline. */
	frame.sf_sc.sc_pc = tf->tf_regs[FRAME_PC];
	frame.sf_sc.sc_ps = tf->tf_regs[FRAME_PS];

	/* Save register context. */
	frametoreg(tf, (struct reg *)frame.sf_sc.sc_regs);
	frame.sf_sc.sc_regs[R_ZERO] = 0xACEDBADE;	/* magic number */
	frame.sf_sc.sc_regs[R_SP] = alpha_pal_rdusp();

 	/* save the floating-point state, if necessary, then copy it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 1);
	frame.sf_sc.sc_ownedfp = l->l_md.md_flags & MDP_FPUSED;
	memcpy((struct fpreg *)frame.sf_sc.sc_fpregs, &l->l_addr->u_pcb.pcb_fp,
	    sizeof(struct fpreg));
	frame.sf_sc.sc_fp_control = alpha_read_fp_c(l);
	memset(frame.sf_sc.sc_reserved, 0, sizeof frame.sf_sc.sc_reserved);
	memset(frame.sf_sc.sc_xxx, 0, sizeof frame.sf_sc.sc_xxx); /* XXX */

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#if defined(COMPAT_13) || defined(COMPAT_OSF1)
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	{
		/* Note: it's a long in the stack frame. */
		sigset13_t mask13;

		native_sigset_to_sigset13(mask, &mask13);
		frame.sf_sc.__sc_mask13 = mask13;
	}
#endif

	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = copyout(&frame, (void *)fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig_sigcontext(%d): copyout failed on sig %d\n",
			       p->p_pid, sig);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig_sigcontext(%d): sig %d usp %p code %x\n",
		       p->p_pid, sig, fp, ksi->ksi_code);
#endif

	/*
	 * Set up the registers to directly invoke the signal handler.  The
	 * signal trampoline is then used to return from the signal.  Note
	 * the trampoline version numbers are coordinated with machine-
	 * dependent code in libc.
	 */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		buildcontext(l,(void *)catcher,
			     (void *)p->p_sigctx.ps_sigcode,
			     (void *)fp);
		break;
#ifdef COMPAT_16
	case 1:
		buildcontext(l,(void *)catcher,
			     (const void *)ps->sa_sigdesc[sig].sd_tramp,
			     (void *)fp);
		break;
#endif
	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	/* sigcontext specific trap frame */
	tf->tf_regs[FRAME_A0] = sig;

	/* tf->tf_regs[FRAME_A1] = ksi->ksi_code; */
	tf->tf_regs[FRAME_A1] = KSI_TRAPCODE(ksi);
	tf->tf_regs[FRAME_A2] = (u_int64_t)&fp->sf_sc;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig(%d): pc %lx, catcher %lx\n", p->p_pid,
		    tf->tf_regs[FRAME_PC], tf->tf_regs[FRAME_A3]);
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		    p->p_pid, sig);
#endif
}

#ifdef COMPAT_16 /* not needed if COMPAT_OSF1 only */
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
	struct proc *p = l->l_proc;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
	    printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if (ALIGN(scp) != (u_int64_t)scp)
		return (EINVAL);

	if (copyin((void *)scp, &ksc, sizeof(ksc)) != 0)
		return (EFAULT);

	if (ksc.sc_regs[R_ZERO] != 0xACEDBADE)		/* magic number */
		return (EINVAL);

	/* Restore register context. */
	l->l_md.md_tf->tf_regs[FRAME_PC] = ksc.sc_pc;
	l->l_md.md_tf->tf_regs[FRAME_PS] =
	    (ksc.sc_ps | ALPHA_PSL_USERSET) & ~ALPHA_PSL_USERCLR;

	regtoframe((struct reg *)ksc.sc_regs, l->l_md.md_tf);
	alpha_pal_wrusp(ksc.sc_regs[R_SP]);

	/* XXX ksc.sc_ownedfp ? */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_proc(l, 0);
	memcpy(&l->l_addr->u_pcb.pcb_fp, (struct fpreg *)ksc.sc_fpregs,
	    sizeof(struct fpreg));
	l->l_addr->u_pcb.pcb_fp.fpr_cr = ksc.sc_fpcr;
	l->l_md.md_flags = ksc.sc_fp_control & MDP_FP_C;

	mutex_enter(&p->p_smutex);
	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &ksc.sc_mask, 0);
	mutex_exit(&p->p_smutex);

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}
#endif /* COMPAT_16 */
