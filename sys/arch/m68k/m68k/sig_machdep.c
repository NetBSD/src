/*	$NetBSD: sig_machdep.c,v 1.34.2.1 2007/07/15 13:16:16 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.34.2.1 2007/07/15 13:16:16 ad Exp $");

#include "opt_compat_netbsd.h"

#define __M68K_SIGNAL_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ras.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/ucontext.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/frame.h>

#include <m68k/m68k.h>

extern short exframesize[];
struct fpframe m68k_cached_fpu_idle_frame;
void	m68881_save(struct fpframe *);
void	m68881_restore(struct fpframe *);

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Test for a null floating point frame given a pointer to the start
 * of an fsave'd frame.
 */
#if defined(M68020) || defined(M68030) || defined(M68040)
#if defined(M68060)
#define	FPFRAME_IS_NULL(fp)						    \
	    ((fputype == FPU_68060 &&					    \
	      ((struct fpframe060 *)(fp))->fpf6_frmfmt == FPF6_FMT_NULL) || \
	     (fputype != FPU_68060  &&					    \
	      ((union FPF_u1 *)(fp))->FPF_nonnull.FPF_version == 0))
#else
#define FPFRAME_IS_NULL(fp) \
	    (((union FPF_u1 *)(fp))->FPF_nonnull.FPF_version == 0)
#endif
#else
#define FPFRAME_IS_NULL(fp) \
	    (((struct fpframe060 *)(fp))->fpf6_frmfmt == FPF6_FMT_NULL)
#endif

void *
getframe(struct lwp *l, int sig, int *onstack)
{
	struct frame *tf = (struct frame *)l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	*onstack =(l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
		&& (SIGACTION(l->l_proc, sig).sa_flags & SA_ONSTACK) != 0;

	if (*onstack)
		return (char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size;
	else
		return (void *)tf->f_regs[SP];
}

/*
 * Build context to run handler in.  We invoke the handler
 * directly, only returning via the trampoline.  Note the
 * trampoline version numbers are coordinated with machine-
 * dependent code in libc.
 */
void
buildcontext(struct lwp *l, void *catcher, void *fp)
{
	struct frame *frame = (struct frame *)l->l_md.md_regs;

	/*
	 * Set up the registers to return to the signal handler.  The
	 * handler will then return to the signal trampoline.
	 */
	frame->f_regs[SP] = (int)fp;
	frame->f_pc = (int)catcher;
}

static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo *fp = getframe(l, sig, &onstack), kf;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	fp--;

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* handled by sendsig_sigcontext */
	case 1:		/* handled by sendsig_sigcontext */
	default:	/* unknown version */
		printf("nsendsig: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	case 2:
		break;
	}

	kf.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
	kf.sf_signum = sig;
	kf.sf_sip = &fp->sf_si;
	kf.sf_ucp = &fp->sf_uc;
	kf.sf_si._info = ksi->ksi_info;
	kf.sf_uc.uc_flags = _UC_SIGMASK;
	kf.sf_uc.uc_sigmask = *mask;
	kf.sf_uc.uc_link = l->l_ctxlink;
	kf.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&kf.sf_uc.uc_stack, 0, sizeof(kf.sf_uc.uc_stack));
	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	cpu_getmcontext(l, &kf.sf_uc.uc_mcontext, &kf.sf_uc.uc_flags);
	error = copyout(&kf, fp, sizeof(kf));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, catcher, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{

#ifdef COMPAT_16
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else
#endif
		sendsig_siginfo(ksi, mask);
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, u_int *flags)
{
	__greg_t *gr = mcp->__gregs;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	unsigned int format = frame->f_format;
	__greg_t ras_pc;

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

	if ((ras_pc = (__greg_t)ras_lookup(l->l_proc,
	    (void *) gr[_REG_PC])) != -1)
		gr[_REG_PC] = ras_pc;

	*flags |= _UC_CPU;

	/* Save exception frame information. */
	mcp->__mc_pad.__mc_frame.__mcf_format = format;
	if (format >= FMT4) {
		mcp->__mc_pad.__mc_frame.__mcf_vector = frame->f_vector;
		(void)memcpy(&mcp->__mc_pad.__mc_frame.__mcf_exframe,
		    &frame->F_u, (size_t)exframesize[format]);

		/* Leave indicators, see above. */
		frame->f_stackadj += exframesize[format];
		frame->f_format = frame->f_vector = 0;
	}

	if (fputype != FPU_NONE) {
		/* Save FPU context. */
		struct fpframe *fpf = &l->l_addr->u_pcb.pcb_fpregs;

		/*
		 * If we're dealing with the current lwp, we need to
		 * save its FP state. Otherwise, its state is already
		 * store in its PCB.
		 */
		if (l == curlwp)
			m68881_save(fpf);

		mcp->__mc_pad.__mc_frame.__mcf_fpf_u1 = fpf->FPF_u1;

		/* If it's a null frame there's no need to save/convert. */
		if (!FPFRAME_IS_NULL(fpf)) {
			mcp->__mc_pad.__mc_frame.__mcf_fpf_u2 = fpf->FPF_u2;
			(void)memcpy(mcp->__fpregs.__fp_fpregs,
			    fpf->fpf_regs, sizeof(fpf->fpf_regs));
			mcp->__fpregs.__fp_pcr = fpf->fpf_fpcr;
			mcp->__fpregs.__fp_psr = fpf->fpf_fpsr;
			mcp->__fpregs.__fp_piaddr = fpf->fpf_fpiar;
			*flags |= _UC_FPU;
		}
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, u_int flags)
{
	const __greg_t *gr = mcp->__gregs;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	unsigned int format = mcp->__mc_pad.__mc_frame.__mcf_format;
	int sz;

	/* Validate the supplied context */
	if (((flags & _UC_CPU) != 0 &&
	     (gr[_REG_PS] & (PSL_MBZ|PSL_IPL|PSL_S)) != 0))
		return (EINVAL);

	/* Restore exception frame information if necessary. */
	if ((flags & _UC_M68K_UC_USER) == 0 && format >= FMT4) {
		if (format > FMTB)
			return (EINVAL);
		sz = exframesize[format];
		if (sz < 0)
			return (EINVAL);

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
		frame->f_vector = mcp->__mc_pad.__mc_frame.__mcf_vector;
		(void)memcpy(&frame->F_u,
		    &mcp->__mc_pad.__mc_frame.__mcf_exframe, (size_t)sz);
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
		frame->f_sr = gr[_REG_PS];
		frame->f_pc = gr[_REG_PC];
	}

	if (fputype != FPU_NONE) {
		const __fpregset_t *fpr = &mcp->__fpregs;
		struct fpframe *fpf = &l->l_addr->u_pcb.pcb_fpregs;

		switch (flags & (_UC_FPU | _UC_M68K_UC_USER)) {
		case _UC_FPU:
			/*
			 * We're restoring FPU context saved by the above
			 * cpu_getmcontext(). We can do a full frestore if
			 * something other than an null frame was saved.
			 */
			fpf->FPF_u1 = mcp->__mc_pad.__mc_frame.__mcf_fpf_u1;
			if (!FPFRAME_IS_NULL(fpf)) {
				fpf->FPF_u2 =
				    mcp->__mc_pad.__mc_frame.__mcf_fpf_u2;
				(void)memcpy(fpf->fpf_regs,
				    fpr->__fp_fpregs, sizeof(fpf->fpf_regs));
				fpf->fpf_fpcr = fpr->__fp_pcr;
				fpf->fpf_fpsr = fpr->__fp_psr;
				fpf->fpf_fpiar = fpr->__fp_piaddr;
			}
			break;

		case _UC_FPU | _UC_M68K_UC_USER:
			/*
			 * We're restoring FPU context saved by the
			 * userland _getcontext_() function. Since there
			 * is no FPU frame to restore. We assume the FPU was
			 * "idle" when the frame was created, so use the
			 * cached idle frame.
			 */
			fpf->FPF_u1 = m68k_cached_fpu_idle_frame.FPF_u1;
			fpf->FPF_u2 = m68k_cached_fpu_idle_frame.FPF_u2;
			(void)memcpy(fpf->fpf_regs, fpr->__fp_fpregs,
			    sizeof(fpf->fpf_regs));
			fpf->fpf_fpcr = fpr->__fp_pcr;
			fpf->fpf_fpsr = fpr->__fp_psr;
			fpf->fpf_fpiar = fpr->__fp_piaddr;
			break;

		default:
			/*
			 * The saved context contains no FPU state.
			 * Restore a NULL frame.
			 */
			fpf->FPF_u1.FPF_null = 0;
			break;
		}

		/*
		 * We only need to restore FP state right now if we're
		 * dealing with curlwp. Otherwise, it'll be restored
		 * (from the PCB) when this lwp is given the CPU.
		 */
		if (l == curlwp)
			m68881_restore(fpf);
	}

	mutex_enter(&l->l_proc->p_smutex);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(&l->l_proc->p_smutex);

	return 0;
}
