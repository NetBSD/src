/*    $NetBSD: compat_16_machdep.c,v 1.2 2003/09/25 22:04:17 christos Exp $   */

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
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.2 2003/09/25 22:04:17 christos Exp $");

#include "opt_compat_netbsd.h"

#define __M68K_SIGNAL_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/ucontext.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/frame.h>

extern  short exframesize[];
int	compat_16_sys___sigreturn14(struct lwp *, void *, register_t *);
void	m68881_save __P((struct fpframe *));
void	m68881_restore __P((struct fpframe *));

#ifdef DEBUG
extern int sigdebug;
extern int sigpid;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

#ifdef COMPAT_16
/*
 * Send an interrupt to process.
 */
void
sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	int onstack;
	int sig = ksi->ksi_signo;
	u_long code = ksi->ksi_trap;
	struct sigframe_sigcontext *fp = getframe(l, sig, &onstack), kf;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	short ft = frame->f_format;

	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p ft %d\n",
		       p->p_pid, sig, &onstack, fp, &fp->sf_sc, ft);
#endif

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		kf.sf_ra = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		kf.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	kf.sf_signum = sig;
	kf.sf_code = code;
	kf.sf_scp = &fp->sf_sc;

	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- general registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	kf.sf_state.ss_flags = SS_USERREGS;
	memcpy(kf.sf_state.ss_frame.f_regs, frame->f_regs,
	    sizeof(frame->f_regs));
	if (ft >= FMT4) {
#ifdef DEBUG
		if (ft > 15 || exframesize[ft] < 0)
			panic("sendsig: bogus frame type");
#endif
		kf.sf_state.ss_flags |= SS_RTEFRAME;
		kf.sf_state.ss_frame.f_format = frame->f_format;
		kf.sf_state.ss_frame.f_vector = frame->f_vector;
		memcpy(&kf.sf_state.ss_frame.F_u, &frame->F_u,
		    (size_t) exframesize[ft]);
		/*
		 * Leave an indicator that we need to clean up the kernel
		 * stack.  We do this by setting the "pad word" above the
		 * hardware stack frame to the amount the stack must be
		 * adjusted by.
		 *
		 * N.B. we increment rather than just set f_stackadj in
		 * case we are called from syscall when processing a
		 * sigreturn.  In that case, f_stackadj may be non-zero.
		 */
		frame->f_stackadj += exframesize[ft];
		frame->f_format = frame->f_vector = 0;
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sendsig(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}

	if (fputype) {
		kf.sf_state.ss_flags |= SS_FPSTATE;
		m68881_save(&kf.sf_state.ss_fpstate);
	}
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kf.sf_state.ss_fpstate)
		printf("sendsig(%d): copy out FP state (%x) to %p\n",
		       p->p_pid, *(u_int *)&kf.sf_state.ss_fpstate,
		       &kf.sf_state.ss_fpstate);
#endif

	/* Build the signal context to be used by sigreturn. */
	kf.sf_sc.sc_sp = frame->f_regs[SP];
	kf.sf_sc.sc_fp = frame->f_regs[A6];
	kf.sf_sc.sc_ap = (int)&fp->sf_state;
	kf.sf_sc.sc_pc = frame->f_pc;
	kf.sf_sc.sc_ps = frame->f_sr;

	/* Save signal stack. */
	kf.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	kf.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &kf.sf_sc.__sc_mask13);
#endif

	if (copyout(&kf, fp, sizeof(kf)) != 0) {
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
		       p->p_pid, sig, kf.sf_scp, fp,
		       kf.sf_sc.sc_sp, kf.sf_sc.sc_ap);
#endif

	buildcontext(l, catcher, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}
#endif

#ifdef COMPAT_16
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
int
compat_16_sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext *scp;
	struct frame *frame;
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
	if ((int)scp & 1)
		return (EINVAL);

	if (copyin(scp, &tsigc, sizeof(tsigc)) != 0)
		return (EFAULT);
	scp = &tsigc;

	/* Make sure the user isn't pulling a fast one on us! */
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return (EINVAL);

	/* Restore register context. */
	frame = (struct frame *) l->l_md.md_regs;

	/*
	 * Grab pointer to hardware state information.
	 * If zero, the user is probably doing a longjmp.
	 */
	if ((rf = scp->sc_ap) == 0)
		goto restore;

	/*
	 * See if there is anything to do before we go to the
	 * expense of copying in close to 1/2K of data
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
		printf("sigreturn(%d): ssp %p usp %x scp %p ft %d\n",
		       p->p_pid, &flags, scp->sc_sp, SCARG(uap, sigcntxp),
		       (flags & SS_RTEFRAME) ? tstate.ss_frame.f_format : -1);
#endif
	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the sigcontext structure.
	 */
	if (flags & SS_RTEFRAME) {
		register int sz;

		/* grab frame type and validate */
		sz = tstate.ss_frame.f_format;
		if (sz > 15 || (sz = exframesize[sz]) < 0
				|| frame->f_stackadj < sz)
			return (EINVAL);
		frame->f_stackadj -= sz;
		frame->f_format = tstate.ss_frame.f_format;
		frame->f_vector = tstate.ss_frame.f_vector;
		memcpy(&frame->F_u, &tstate.ss_frame.F_u, sz);
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, tstate.ss_frame.f_format);
#endif
	}

	/*
	 * Restore most of the users registers except for A6 and SP
	 * which will be handled below.
	 */
	if (flags & SS_USERREGS)
		memcpy(frame->f_regs, tstate.ss_frame.f_regs,
		    sizeof(frame->f_regs) - (2 * NBPW));

	/*
	 * Restore the original FP context
	 */
	if (fputype && (flags & SS_FPSTATE))
		m68881_restore(&tstate.ss_fpstate);

 restore:
	/*
	 * Restore the user supplied information.
	 * This should be at the last so that the error (EINVAL)
	 * is reported to the sigreturn caller, not to the
	 * jump destination.
	 */

	frame->f_regs[SP] = scp->sc_sp;
	frame->f_regs[A6] = scp->sc_fp;
	frame->f_pc = scp->sc_pc;
	frame->f_sr = scp->sc_ps;

	/* Restore signal stack. */
	if (scp->sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &scp->sc_mask, 0);

#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tstate.ss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&tstate.ss_fpstate,
		       &tstate.ss_fpstate);
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}
#endif
