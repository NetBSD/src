/*	$NetBSD: sig_machdep.c,v 1.5 2003/08/31 01:26:36 chs Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: machdep.c 1.74 92/12/20
 *	from: @(#)machdep.c	8.10 (Berkeley) 4/20/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: Utah Hdr: machdep.c 1.74 92/12/20
 *	from: @(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.5 2003/08/31 01:26:36 chs Exp $");

#include "opt_compat_netbsd.h"

#define __HPPA_SIGNAL_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#ifdef DEBUG
int sigdebug = 0xff;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
sendsig(sig, mask, code)
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct sigframe *fp, kf;
	caddr_t sp;
	struct trapframe *tf;
	int onstack, fsize;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	tf = (struct trapframe *)l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * Allocate space for the signal handler context.
	 * The PA-RISC calling convention mandates that
	 * the stack pointer must always be 64-byte aligned,
	 * and points to the first *unused* byte.
	 */
	fsize = sizeof(struct sigframe);
	sp = (onstack ?
	      (caddr_t)p->p_sigctx.ps_sigstk.ss_sp :
	      (caddr_t)tf->tf_sp);
	sp = (caddr_t)(((u_int)(sp + fsize + 63)) & ~63);
	fp = (struct sigframe *) (sp - fsize);

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) && (!sigpid || p->p_pid == sigpid))
		printf("sendsig: %s[%d] sig %d catcher %p\n",
		    p->p_comm, p->p_pid, sig, catcher);
#endif

	/*
	 * Save necessary hardware state.  Currently this includes:
	 *      - original exception frame
	 *      - FP coprocessor state
	 */
	kf.sf_state.ss_flags = SS_USERREGS;
	memcpy(&kf.sf_state.ss_frame, tf, sizeof(*tf));
	/* XXX FP state */

	/* Build the signal context to be used by sigreturn. */
	kf.sf_sc.sc_sp = tf->tf_sp;
	kf.sf_sc.sc_fp = tf->tf_sp;	/* XXX fredette - is this right? */
	kf.sf_sc.sc_ap = (int)&fp->sf_state;
	kf.sf_sc.sc_pcsqh = tf->tf_iisq_head;
	kf.sf_sc.sc_pcoqh = tf->tf_iioq_head;
	kf.sf_sc.sc_pcsqt = tf->tf_iisq_tail;
	kf.sf_sc.sc_pcoqt = tf->tf_iioq_tail;
	kf.sf_sc.sc_ps = tf->tf_ipsw;

	/* Save signal stack. */
	kf.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	kf.sf_sc.sc_mask = *mask;

	/* Fill the calling convention part of the signal frame. */
	kf.sf_psp = 0;
	kf.sf_clup = 0;		/* XXX fredette - is this right? */
	kf.sf_sl = 0;		/* XXX fredette - is this right? */
	kf.sf_edp = 0;		/* XXX fredette - is this right? */

	/* Copy out the signal frame. */
	if (copyout(&kf, fp, fsize)) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig(%d): copyout failed on sig %d\n",
			       p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig(%d): sig %d scp %p fp %p sc_sp %x sc_ap %x\n",
		       p->p_pid, sig, &fp->sf_sc, fp,
		       kf.sf_sc.sc_sp, kf.sf_sc.sc_ap);
#endif

	/* Set up the registers to return to sigcode. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
#if 1 /* COMPAT_16 */
	case 0:		/* legacy on-stack sigtramp */
		tf->tf_iioq_head =
		    (int)p->p_sigctx.ps_sigcode | HPPA_PC_PRIV_USER;
		tf->tf_iioq_tail = tf->tf_iioq_head + 4;
		break;
#endif

	case 1:
		tf->tf_iioq_head =
		    (int)ps->sa_sigdesc[sig].sd_tramp | HPPA_PC_PRIV_USER;
		tf->tf_iioq_tail = tf->tf_iioq_head + 4;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	tf->tf_sp = (int)sp;
	tf->tf_r3 = (int)&fp->sf_sc;
	tf->tf_arg0 = sig;
	tf->tf_arg1 = code;
	tf->tf_arg2 = (int)&fp->sf_sc;
	tf->tf_arg3 = (int)catcher;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	
#ifdef DEBUG    
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif 
}

int
sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext *scp;
	struct trapframe *tf;
	struct sigcontext tsigc;
	struct sigstate tstate;
	int rf, flags;

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
	if ((int)scp & 3)
		return (EINVAL);

	if (copyin(scp, &tsigc, sizeof(tsigc)) != 0)
		return (EFAULT);
	scp = &tsigc;

	/* Make sure the user isn't pulling a fast one on us! */
	/* XXX fredette - until this is done, huge security hole here. */
	/* XXX fredette - requiring that PSL_R be zero will hurt debuggers. */
#define PSW_MBS (PSW_C|PSW_Q|PSW_P|PSW_D|PSW_I)
#define PSW_MBZ (PSW_Y|PSW_Z|PSW_S|PSW_X|PSW_M|PSW_R)
	if ((scp->sc_ps & (PSW_MBS|PSW_MBZ)) != PSW_MBS)
		return (EINVAL);

	/* Restore register context. */
	tf = (struct trapframe *)l->l_md.md_regs;

	/*
	 * Grab pointer to hardware state information.
	 * If zero, the user is probably doing a longjmp.
	 */
	if ((rf = scp->sc_ap) == 0)
		goto restore;

	/*
	 * See if there is anything to do before we go to the
	 * expense of copying in the trapframe
	 */
	flags = fuword((caddr_t)rf);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn(%d): sc_ap %x flags %x\n",
		       p->p_pid, rf, flags);
#endif
	/* fuword failed (bogus sc_ap value). */
	if (flags == -1)
		return (EINVAL);

	if (flags == 0 || copyin((caddr_t)rf, &tstate, sizeof(tstate)) != 0)
		goto restore;
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sigreturn(%d): ssp %p usp %x scp %p\n",
		       p->p_pid, &flags, scp->sc_sp, SCARG(uap, sigcntxp));
#endif

	/*
	 * Restore most of the users registers except for those
	 * in the sigcontext; they will be handled below.
	 */
	if (flags & SS_USERREGS) {

		/*
		 * There are more registers that the user can tell 
		 * us to bash than registers that, for security
		 * or other reasons, we must protect.  So it's
		 * easier (but not faster), to copy these sensitive
		 * register values into the user-provided frame,
		 * then bulk-copy the user-provided frame into
		 * the process' frame.
		 */
#define	SIG_PROTECT(r) tstate.ss_frame.r = tf->r
		/* SRs 5,6,7 must be protected. */
		SIG_PROTECT(tf_sr5);
		SIG_PROTECT(tf_sr6);
		SIG_PROTECT(tf_sr7);

		/* all CRs except CR11 must be protected. */
		SIG_PROTECT(tf_rctr);	/* CR0 */
		/* CRs 1-8 are reserved */
		SIG_PROTECT(tf_pidr1);	/* CR8 */
		SIG_PROTECT(tf_pidr2);	/* CR9 */
		SIG_PROTECT(tf_ccr);	/* CR10 */
		SIG_PROTECT(tf_pidr3);	/* CR12 */
		SIG_PROTECT(tf_pidr4);	/* CR14 */
		SIG_PROTECT(tf_eiem);	/* CR15 */
		/* CR17 is the IISQ head */
		/* CR18 is the IIOQ head */
		SIG_PROTECT(tf_iir);	/* CR19 */
		SIG_PROTECT(tf_isr);	/* CR20 */
		SIG_PROTECT(tf_ior);	/* CR21 */
		/* CR22 is the IPSW */
		SIG_PROTECT(tf_eirr);	/* CR23 */
		SIG_PROTECT(tf_hptm);	/* CR24 */
		SIG_PROTECT(tf_vtop);	/* CR25 */
		/* XXX where are CR26, CR27, CR29, CR31? */
		SIG_PROTECT(tf_cr28);	/* CR28 */
		SIG_PROTECT(tf_cr30);	/* CR30 */
#undef	SIG_PROTECT
		
		/* The bulk copy. */
		*tf = tstate.ss_frame;
	}

	/*
	 * Restore the original FP context
	 */
	/* XXX fredette */

 restore:
	/*
	 * Restore the user supplied information.
	 * This should be at the last so that the error (EINVAL)
	 * is reported to the sigreturn caller, not to the
	 * jump destination.
	 */

	tf->tf_sp = scp->sc_sp;
	/* XXX should we be doing the space registers? */
	tf->tf_iisq_head = scp->sc_pcsqh;
	tf->tf_iioq_head = scp->sc_pcoqh | HPPA_PC_PRIV_USER;
	tf->tf_iisq_tail = scp->sc_pcsqt;
	tf->tf_iioq_tail = scp->sc_pcoqt | HPPA_PC_PRIV_USER;
	tf->tf_ipsw = scp->sc_ps;

	/* Restore signal stack. */
	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &scp->sc_mask, 0);

#ifdef DEBUG
#if 0 /* XXX FP state */
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tstate.ss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&tstate.ss_fpstate,
		       &tstate.ss_fpstate);
#endif
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}

