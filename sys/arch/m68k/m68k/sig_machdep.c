/*	$NetBSD: sig_machdep.c,v 1.15.6.6 2001/11/21 20:25:04 scw Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include "opt_compat_netbsd.h"

#define __M68K_SIGNAL_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/savar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/ucontext.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/frame.h>

#include <m68k/saframe.h>

extern int fputype;
extern short exframesize[];
void	m68881_save __P((struct fpframe *));
void	m68881_restore __P((struct fpframe *));

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct sigframe *fp, kf;
	struct frame *frame;
	short ft;
	int onstack, fsize;

	frame = (struct frame *)l->l_md.md_regs;
	ft = frame->f_format;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	fsize = sizeof(struct sigframe);
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct sigframe *)(frame->f_regs[SP]);
	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p ft %d\n",
		       p->p_pid, sig, &onstack, fp, &fp->sf_sc, ft);
#endif

	/* Build stack frame for signal trampoline. */
	kf.sf_signum = sig;
	kf.sf_code = code;
	kf.sf_scp = &fp->sf_sc;
	kf.sf_handler = catcher;

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
		       p->p_pid, sig, kf.sf_scp, fp,
		       kf.sf_sc.sc_sp, kf.sf_sc.sc_ap);
#endif

	/* Set up the registers to return to sigcode. */
	frame->f_regs[SP] = (int)fp;
	frame->f_pc = (int)p->p_sigctx.ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}

void
cpu_upcall(l)
	struct lwp *l;
{
	extern char sigcode[], upcallcode[];
	struct proc *p = l->l_proc;
	struct sadata *sd = p->p_sa;
	struct saframe *sfp, sf;
	struct sa_t **sapp, *sap;
	struct sa_t self_sa, e_sa, int_sa;
	struct sa_t *sas[3];
	struct sadata_upcall *sau;
	struct frame *frame;
	void *stack, *ap;
	ucontext_t u, *up;
	int i, nsas, nevents, nint;
	int x, y;

	frame = (struct frame *)l->l_md.md_regs;

	KDASSERT(LIST_EMPTY(&sd->sa_upcalls) == 0);

	sau = LIST_FIRST(&sd->sa_upcalls);

	stack = (char *)sau->sau_stack.ss_sp + sau->sau_stack.ss_size;

	self_sa.sa_id = l->l_lid;
	self_sa.sa_cpu = 0;
	sas[0] = &self_sa;
	nsas = 1;

	nevents = 0;
	if (sau->sau_event) {
		e_sa.sa_context = cpu_stashcontext(sau->sau_event);
		e_sa.sa_id = sau->sau_event->l_lid;
		e_sa.sa_cpu = 0;
		sas[nsas++] = &e_sa;
		nevents = 1;
	}

	nint = 0;
	if (sau->sau_interrupted) {
		int_sa.sa_context = cpu_stashcontext(sau->sau_interrupted);
		int_sa.sa_id = sau->sau_interrupted->l_lid;
		int_sa.sa_cpu = 0;
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
		sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
		printf("cpu_upcall: couldn't copyout activation ucontext"
		    " for %d.%d\n", l->l_proc->p_pid, l->l_lid);
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
			/* Copying onto the stack didn't work.  Die. */
			sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
			printf("cpu_upcall: couldn't copyout sa_t %d for "
			    "%d.%d (x=%d, y=%d)\n",
			    i, l->l_proc->p_pid, l->l_lid, x, y);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	}

	/*
	 * Copy out the arg, if any.
	 */
	if (sau->sau_arg) {
		ap = (char *)sapp - sau->sau_argsize;
		sfp = (struct saframe *)ap - 1;
		if (copyout(sau->sau_arg, ap, sau->sau_argsize) != 0) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	} else {
		ap = NULL;
		sfp = (struct saframe *)sapp - 1;
	}

	/* Finally, copy out the rest of the frame */
	sf.sa_type = sau->sau_type;
	sf.sa_sas = sapp;
	sf.sa_events = nevents;
	sf.sa_interrupted = nint;
	sf.sa_arg = ap;
	sf.sa_upcall = sd->sa_upcall;

	sadata_upcall_free(sau);

	if (copyout(&sf, sfp, sizeof(sf)) != 0) {
		/* Copying onto the stack didn't work. Die. */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* XXX hack-o-matic */
	frame->f_pc = (int)((caddr_t) p->p_sigctx.ps_sigcode + (
		(caddr_t)upcallcode - (caddr_t)sigcode));
	frame->f_regs[SP] = (int) sfp;
	frame->f_regs[A6] = 0; /* indicate call-frame-top to debuggers */
	frame->f_sr &= ~PSL_T;
}

/* Save the user-level ucontext_t on the LWP's own stack. */
ucontext_t *
cpu_stashcontext(struct lwp *l)
{
	ucontext_t u, *up;
	struct frame *frame;
	void *stack;

	frame = (struct frame *)l->l_md.md_regs;
	stack = (char *)frame->f_regs[SP] - sizeof(ucontext_t);
	getucontext(l, &u);
	up = stack;

	if (copyout(&u, stack, sizeof(ucontext_t)) != 0) {
		/* Copying onto the stack didn't work. Die. */
#ifdef DIAGNOSTIC
		printf("cpu_stashcontext: couldn't copyout context of %d.%d\n",
		    l->l_proc->p_pid, l->l_lid);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	return up;
}
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

void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	__greg_t *gr = mcp->__gregs;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	unsigned int format = frame->f_format;

	/* Save general registers. */
	gr[_REG_D0] = frame->f_regs[D0];
	gr[_REG_D1] = frame->f_regs[D1];
	gr[_REG_D2] = frame->f_regs[D2];
	gr[_REG_D3] = frame->f_regs[D3];
	gr[_REG_D4] = frame->f_regs[D4];
	gr[_REG_D5] = frame->f_regs[D5];
	gr[_REG_D6] = frame->f_regs[D6];
	gr[_REG_D7] = frame->f_regs[D7];
	gr[_REG_A0] = frame->f_regs[A0];
	gr[_REG_A1] = frame->f_regs[A1];
	gr[_REG_A2] = frame->f_regs[A2];
	gr[_REG_A3] = frame->f_regs[A3];
	gr[_REG_A4] = frame->f_regs[A4];
	gr[_REG_A5] = frame->f_regs[A5];
	gr[_REG_A6] = frame->f_regs[A6];
	gr[_REG_A7] = frame->f_regs[SP];
	gr[_REG_PS] = frame->f_sr;
	gr[_REG_PC] = frame->f_pc;
	*flags |= _UC_CPU;

	/* Save exception frame information. */
	if (format >= FMT4) {
		mcp->__mc_pad.mc_frame.format = format;
		mcp->__mc_pad.mc_frame.vector = frame->f_vector;
		(void)memcpy(&mcp->__mc_pad.mc_frame.exframe, &frame->F_u,
		    (size_t)exframesize[format]);
		
		/* Leave indicators, see above. */
		frame->f_stackadj += exframesize[format];
		frame->f_format = frame->f_vector = 0;
		*flags |= _UC_M68K_FMT_VALID;
	}

	if (fputype != FPU_NONE) {
		/* Save FPU context. */
		struct fpframe fpf;

		/* At the very least, we need to save a null frame */
		m68881_save(&fpf);
		mcp->__mc_pad.mc_frame.fpf_u1 = fpf.FPF_u1;

		/* If it's a null frame there's no need to save/convert. */
		if (!FPFRAME_IS_NULL(&fpf)) {
			__fpregset_t *fpr = &mcp->__fpregs;

			fpr->__fp_pcr    = fpf.fpf_fpcr;
			fpr->__fp_psr    = fpf.fpf_fpsr;
			fpr->__fp_piaddr = fpf.fpf_fpiar;
			(void)memcpy(&fpr->__fp_fpregs, &fpf.fpf_regs,
			    sizeof (fpr->__fp_fpregs));
			mcp->__mc_pad.mc_frame.fpf_u2 = fpf.FPF_u2;
		}

		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	__greg_t *gr = mcp->__gregs;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	unsigned int format = mcp->__mc_pad.mc_frame.format;
	int sz;

	/* Validate the supplied context */
	if (((flags & _UC_CPU) != 0 &&
	     (gr[_REG_PS] & (PSL_MBZ|PSL_IPL|PSL_S)) != 0) ||
	    ((flags & _UC_M68K_FMT_VALID) != 0 &&
	     (format > FMTB || (sz = exframesize[format]) < 0)))
		return (EINVAL);

	/* Restore exception frame information if necessary. */
	if ((flags & _UC_M68K_FMT_VALID) != 0 && format >= FMT4) {
		if (frame->f_stackadj == 0) {
			reenter_syscall(frame, sz);
			/* NOTREACHED */
		}

#ifdef DIAGNOSTIC
		if (sz != frame->f_stackadj)
			panic("cpu_setmcontext: %d != %d",
			    sz, frame->f_stackadj);
#endif

		frame->f_format = format;
		frame->f_vector = mcp->__mc_pad.mc_frame.vector;
		(void)memcpy(&frame->F_u,
		    &mcp->__mc_pad.mc_frame.exframe, (size_t)sz);
		frame->f_stackadj -= sz;
	}

	if ((flags & _UC_CPU) != 0) {
		/* Restore general registers. */
		frame->f_regs[D0] = gr[_REG_D0];
		frame->f_regs[D1] = gr[_REG_D1];
		frame->f_regs[D2] = gr[_REG_D2];
		frame->f_regs[D3] = gr[_REG_D3];
		frame->f_regs[D4] = gr[_REG_D4];
		frame->f_regs[D5] = gr[_REG_D5];
		frame->f_regs[D6] = gr[_REG_D6];
		frame->f_regs[D7] = gr[_REG_D7];
		frame->f_regs[A0] = gr[_REG_A0];
		frame->f_regs[A1] = gr[_REG_A1];
		frame->f_regs[A2] = gr[_REG_A2];
		frame->f_regs[A3] = gr[_REG_A3];
		frame->f_regs[A4] = gr[_REG_A4];
		frame->f_regs[A5] = gr[_REG_A5];
		frame->f_regs[A6] = gr[_REG_A6];
		frame->f_regs[SP] = gr[_REG_A7];
		frame->f_sr       = gr[_REG_PS];
		frame->f_pc       = gr[_REG_PC];
	}

	if ((flags & _UC_FPU) != 0 && fputype != FPU_NONE) {
		/* Restore Floating Point State */
		struct fpframe fpf;

		/* Copy the first section of the FP frame */
		fpf.FPF_u1 = mcp->__mc_pad.mc_frame.fpf_u1;

		/* Only copy the remainder if it's not a null frame */
		if (!FPFRAME_IS_NULL(&fpf)) {
			const __fpregset_t *fpr = &mcp->__fpregs;
			fpf.fpf_fpcr  = fpr->__fp_pcr;
			fpf.fpf_fpsr  = fpr->__fp_psr;
			fpf.fpf_fpiar = fpr->__fp_piaddr;
			(void)memcpy(&fpf.fpf_regs, &fpr->__fp_fpregs,
			    sizeof (fpf.fpf_regs));
			fpf.FPF_u2 = mcp->__mc_pad.mc_frame.fpf_u2;
		}

		m68881_restore(&fpf);
	}

	return (0);
}
