/*	$NetBSD: linux_machdep.c,v 1.43 2021/09/07 11:43:04 riastradh Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.43 2021/09/07 11:43:04 riastradh Exp $");

#define COMPAT_LINUX 1

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>

/* XXX should be in an include file somewhere */
#define CC_PURGE	1
#define CC_FLUSH	2
#define CC_IPURGE	4
#define CC_EXTPURGE	0x80000000
/* XXX end should be */

extern short exframesize[];

#ifdef DEBUG
extern int sigdebug;
extern int sigpid;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

void setup_linux_sigframe(struct frame *frame, int sig,
    const sigset_t *mask, void *usp);
void setup_linux_rt_sigframe(struct frame *frame, int sig,
    const sigset_t *mask, void *usp, struct lwp *l);

/*
 * Deal with some m68k-specific things in the Linux emulation code.
 */

/*
 * Setup registers on program execution.
 */
void
linux_setregs(struct lwp *l, struct exec_package *epp, vaddr_t stack)
{

	setregs(l, epp, stack);
}

/*
 * Setup signal frame for old signal interface.
 */
void
setup_linux_sigframe(struct frame *frame, int sig, const sigset_t *mask, void *usp)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct linux_sigframe *fp, kf;
	short ft;
	int error;

	ft = frame->f_format;

	/* Allocate space for the signal handler context on the user stack. */
	fp = (struct linux_sigframe *) usp;
	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("setup_linux_sigframe(%d): sig %d ssp %p usp %p scp %p ft %d\n",
		       p->p_pid, sig, &ft, fp, &fp->sf_c.c_sc, ft);
#endif

	memset(&kf, 0, sizeof(kf));

	/* Build stack frame. */
	kf.sf_psigtramp = fp->sf_sigtramp;	/* return addr for handler */
	kf.sf_signum = native_to_linux_signo[sig];
	kf.sf_code = frame->f_vector;		/* Does anyone use it? */
	kf.sf_scp = &fp->sf_c.c_sc;

	/* The sigtramp code is on the stack frame on Linux/m68k. */
	kf.sf_sigtramp[0] = LINUX_SF_SIGTRAMP0;
	kf.sf_sigtramp[1] = LINUX_SF_SIGTRAMP1;

	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- scratch registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	kf.sf_c.c_sc.sc_d0 = frame->f_regs[D0];
	kf.sf_c.c_sc.sc_d1 = frame->f_regs[D1];
	kf.sf_c.c_sc.sc_a0 = frame->f_regs[A0];
	kf.sf_c.c_sc.sc_a1 = frame->f_regs[A1];

	/* Clear for security (and initialize ss_format). */
	memset(&kf.sf_c.c_sc.sc_ss, 0, sizeof kf.sf_c.c_sc.sc_ss);

	if (ft >= FMT4) {
#ifdef DEBUG
		if (ft > 15 || exframesize[ft] < 0)
			panic("setup_linux_sigframe: bogus frame type");
#endif
		kf.sf_c.c_sc.sc_ss.ss_format = ft;
		kf.sf_c.c_sc.sc_ss.ss_vector = frame->f_vector;
		memcpy( &kf.sf_c.c_sc.sc_ss.ss_frame, &frame->F_u,
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
			printf("setup_linux_sigframe(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}

	switch (fputype) {
	case FPU_NONE:
		break;
#ifdef M68060
	case FPU_68060:
		__asm("fsave %0" : "=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.FPF_u1)
			: : "memory");
		if (((struct fpframe060 *)&kf.sf_c.c_sc.sc_ss.ss_fpstate.FPF_u1)
					->fpf6_frmfmt != FPF6_FMT_NULL) {
			__asm("fmovem %%fp0-%%fp1,%0" :
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_regs[0][0]));
			/*
			 * On 060,  "fmovem fpcr/fpsr/fpi,<ea>"  is
			 * emulated by software and slow.
			 */
			__asm("fmovem %%fpcr,%0; fmovem %%fpsr,%1; fmovem %%fpi,%2" :
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_fpcr),
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_fpsr),
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_fpiar));
		}
		break;
#endif
	default:
		__asm("fsave %0" : "=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.FPF_u1)
			: : "memory");
		if (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_version) {
			__asm("fmovem %%fp0-%%fp1,%0; fmovem %%fpcr/%%fpsr/%%fpi,%1" :
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_regs[0][0]),
				"=m" (kf.sf_c.c_sc.sc_ss.ss_fpstate.fpf_fpcr)
				: : "memory");
		}
		break;
	}
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kf.sf_c.c_sc.sc_ss.ss_fpstate)
		printf("setup_linux_sigframe(%d): copy out FP state (%x) to %p\n",
		       p->p_pid, *(u_int *)&kf.sf_c.c_sc.sc_ss.ss_fpstate,
		       &kf.sf_c.c_sc.sc_ss.ss_fpstate);
#endif

	/* Build the signal context to be used by sigreturn. */
#if LINUX__NSIG_WORDS > 1
	native_to_linux_old_extra_sigset(&kf.sf_c.c_sc.sc_mask,
	    kf.sf_c.c_extrasigmask, mask);
#else
	native_to_linux_old_sigset(&kf.sf_c.c_sc.sc_mask, mask);
#endif
	kf.sf_c.c_sc.sc_sp = frame->f_regs[SP];
	kf.sf_c.c_sc.sc_pc = frame->f_pc;
	kf.sf_c.c_sc.sc_ps = frame->f_sr;
	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	error = copyout(&kf, fp, sizeof(struct linux_sigframe));
	mutex_enter(p->p_lock);

	if (error) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("setup_linux_sigframe(%d): copyout failed on sig %d\n",
			       p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it a segmentation
		 * violation to halt it in its tracks.
		 */
		sigexit(l, SIGSEGV);
		/* NOTREACHED */
	}

	/*
	 * The signal trampoline is on the signal frame.
	 * Clear the instruction cache in case of cached.
	 */
	cachectl1(CC_EXTPURGE | CC_IPURGE,
			(vaddr_t) fp->sf_sigtramp, sizeof fp->sf_sigtramp, p);

	/* Set up the user stack pointer. */
	frame->f_regs[SP] = (int)fp;

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("setup_linux_sigframe(%d): sig %d scp %p fp %p sc_sp %x\n",
		       p->p_pid, sig, kf.sf_scp, fp, kf.sf_c.c_sc.sc_sp);
#endif
}

/*
 * Setup signal frame for new RT signal interface.
 */
void
setup_linux_rt_sigframe(struct frame *frame, int sig, const sigset_t *mask, void *usp, struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct linux_rt_sigframe *fp, kf;
	int error;
	short ft;

	ft = frame->f_format;

	/* Allocate space for the signal handler context on the user stack. */
	fp = (struct linux_rt_sigframe *) usp;
	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("setup_linux_rt_sigframe(%d): sig %d ssp %p usp %p ucp %p ft %d\n",
		       p->p_pid, sig, &ft, fp, &fp->sf_uc, ft);
#endif

	memset(&kf, 0, sizeof(kf));

	/* Build stack frame. */
	kf.sf_psigtramp = fp->sf_sigtramp;	/* return addr for handler */
	kf.sf_signum = native_to_linux_signo[sig];
	kf.sf_pinfo = &fp->sf_info;
	kf.sf_puc = &fp->sf_uc;

	/* The sigtramp code is on the stack frame on Linux/m68k. */
	kf.sf_sigtramp[0] = LINUX_RT_SF_SIGTRAMP0;
	kf.sf_sigtramp[1] = LINUX_RT_SF_SIGTRAMP1;

	/* clear for security (and initialize uc_flags, ss_format, etc.). */
	memset(&kf.sf_uc, 0, sizeof(struct linux_ucontext));

	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- general registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	/* version of mcontext */
	kf.sf_uc.uc_mc.mc_version = LINUX_MCONTEXT_VERSION;

	/* general registers and pc/sr */
	memcpy( kf.sf_uc.uc_mc.mc_gregs.gr_regs, frame->f_regs, sizeof(u_int)*16);
	kf.sf_uc.uc_mc.mc_gregs.gr_pc = frame->f_pc;
	kf.sf_uc.uc_mc.mc_gregs.gr_sr = frame->f_sr;

	if (ft >= FMT4) {
#ifdef DEBUG
		if (ft > 15 || exframesize[ft] < 0)
			panic("setup_linux_rt_sigframe: bogus frame type");
#endif
		kf.sf_uc.uc_ss.ss_format = ft;
		kf.sf_uc.uc_ss.ss_vector = frame->f_vector;
		memcpy( &kf.sf_uc.uc_ss.ss_frame, &frame->F_u,
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
			printf("setup_linux_rt_sigframe(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}

	switch (fputype) {
	case FPU_NONE:
		break;
#ifdef M68060
	case FPU_68060:
		__asm("fsave %0" : "=m" (kf.sf_uc.uc_ss.ss_fpstate));
				/* See note below. */
		if (((struct fpframe060 *) &kf.sf_uc.uc_ss.ss_fpstate.FPF_u1)
					->fpf6_frmfmt != FPF6_FMT_NULL) {
			__asm("fmovem %%fp0-%%fp7,%0" :
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_regs[0][0]));
			/*
			 * On 060,  "fmovem fpcr/fpsr/fpi,<ea>"  is
			 * emulated by software and slow.
			 */
			__asm("fmovem %%fpcr,%0; fmovem %%fpsr,%1; fmovem %%fpi,%2" :
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_fpcr),
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_fpsr),
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_fpiar));
		}
		break;
#endif
	default:
		/*
		 * NOTE:  We give whole of the  "struct linux_rt_fpframe"
		 * to the __asm("fsave") argument; not the FPF_u1 element only.
		 * Unlike the non-RT version of this structure,
		 * this contains only the FPU state used by "fsave"
		 * (and whole of the information is in the structure).
		 * This gives the correct dependency information to the __asm(),
		 * and no "memory" is required to the ``clobberd'' list.
		 */
		__asm("fsave %0" : "=m" (kf.sf_uc.uc_ss.ss_fpstate));
		if (kf.sf_uc.uc_ss.ss_fpstate.fpf_version) {
			__asm("fmovem %%fp0-%%fp7,%0; fmovem %%fpcr/%%fpsr/%%fpi,%1" :
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_regs[0][0]),
				"=m" (kf.sf_uc.uc_mc.mc_fpregs.fpr_fpcr)
				: : "memory");
		}
		break;
	}
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kf.sf_uc.uc_ss.ss_fpstate)
		printf("setup_linux_rt_sigframe(%d): copy out FP state (%x) to %p\n",
		       p->p_pid, *(u_int *)&kf.sf_uc.uc_ss.ss_fpstate,
		       &kf.sf_uc.uc_ss.ss_fpstate);
#endif

	/*
	 * XXX XAX Create bogus siginfo data.  This can't really
	 * XXX be fixed until NetBSD has realtime signals.
	 * XXX Or we do the emuldata thing.
	 * XXX -erh
	 */
	memset(&kf.sf_info, 0, sizeof(struct linux_siginfo));
	kf.sf_info.lsi_signo = sig;
	kf.sf_info.lsi_code = LINUX_SI_USER;
	kf.sf_info.lsi_pid = p->p_pid;
	kf.sf_info.lsi_uid = kauth_cred_geteuid(l->l_cred);	/* Use real uid here? */

	/* Build the signal context to be used by sigreturn. */
	native_to_linux_sigset(&kf.sf_uc.uc_sigmask, mask);
	kf.sf_uc.uc_stack.ss_sp = l->l_sigstk.ss_sp;
	kf.sf_uc.uc_stack.ss_flags =
		(l->l_sigstk.ss_flags & SS_ONSTACK ? LINUX_SS_ONSTACK : 0) |
		(l->l_sigstk.ss_flags & SS_DISABLE ? LINUX_SS_DISABLE : 0);
	kf.sf_uc.uc_stack.ss_size = l->l_sigstk.ss_size;
	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	error = copyout(&kf, fp, sizeof(struct linux_rt_sigframe));
	mutex_enter(p->p_lock);

	if (error) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("setup_linux_rt_sigframe(%d): copyout failed on sig %d\n",
			       p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it a segmentation
		 * violation to halt it in its tracks.
		 */
		sigexit(l, SIGSEGV);
		/* NOTREACHED */
	}

	/*
	 * The signal trampoline is on the signal frame.
	 * Clear the instruction cache in case of cached.
	 */
	cachectl1(CC_EXTPURGE | CC_IPURGE,
			(vaddr_t) fp->sf_sigtramp, sizeof fp->sf_sigtramp, p);

	/* Set up the user stack pointer. */
	frame->f_regs[SP] = (int)fp;

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("setup_linux_rt_sigframe(%d): sig %d puc %p fp %p sc_sp %x\n",
		       p->p_pid, sig, kf.sf_puc, fp,
		       kf.sf_uc.uc_mc.mc_gregs.gr_regs[SP]);
#endif
}

/*
 * Send an interrupt to Linux process.
 */
void
linux_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	/* u_long code = ksi->ksi_trap; */
	int sig = ksi->ksi_signo;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	int onstack;
	/* user stack for signal context */
	void *usp = getframe(l, sig, &onstack);
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	/* Setup the signal frame (and part of the trapframe). */
	if (SIGACTION(p, sig).sa_flags & SA_SIGINFO)
		setup_linux_rt_sigframe(frame, sig, mask, usp, l);
	else
		setup_linux_sigframe(frame, sig, mask, usp);

	/* Call the signal handler. */
	frame->f_pc = (u_int) catcher;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("linux_sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}

/*
 * The linux_sys_sigreturn and linux_sys_rt_sigreturn
 * system calls cleanup state after a signal
 * has been taken.  Reset signal mask and stack
 * state from context left by linux_sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by linux_sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 *
 * Note that the sigreturn system calls of Linux/m68k
 * do not return on errors, but issue segmentation
 * violation and terminate the process.
 */
/* ARGSUSED */
int
linux_sys_sigreturn(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct frame *frame;
	struct linux_sigc2 tsigc2;	/* extra mask and sigcontext */
	struct linux_sigcontext *scp;	/* pointer to sigcontext */
	sigset_t mask;
	int sz = 0;			/* extra frame size */
	int usp;

	/*
	 * sigreturn of Linux/m68k takes no arguments.
	 * The user stack points at struct linux_sigc2.
	 */
	frame = (struct frame *) l->l_md.md_regs;
	usp = frame->f_regs[SP];
	if (usp & 1)
		goto bad;

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("linux_sys_sigreturn: pid %d, usp %p\n",
			p->p_pid, (void *) usp);
#endif

	/* Grab whole of the sigcontext. */
	if (copyin((void *) usp, &tsigc2, sizeof tsigc2)) {
bad:
		mutex_enter(p->p_lock);
		sigexit(l, SIGSEGV);
	}

	scp = &tsigc2.c_sc;

	/*
	 * Check kernel stack and re-enter to syscall() if needed.
	 */
	if ((sz = scp->sc_ss.ss_format) != 0) {
		if ((sz = exframesize[sz]) < 0)
			goto bad;
		if (sz && frame->f_stackadj == 0) {
			/*
			 * Extra stack space is required but not allocated.
			 * Allocate and re-enter syscall().
			 */
			reenter_syscall(frame, sz);
			/* NOTREACHED */
		}
	}
#ifdef DEBUG
	/* reenter_syscall() doesn't adjust stack. */
	if (sz != frame->f_stackadj)
		panic("linux_sys_sigreturn: adj: %d != %d",
			sz, frame->f_stackadj);
#endif

	mutex_enter(p->p_lock);

	/* Restore signal stack. */
	l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
#if LINUX__NSIG_WORDS > 1
	linux_old_extra_to_native_sigset(&mask, &scp->sc_mask,
					 tsigc2.c_extrasigmask);
#else
	linux_old_to_native_sigset(&scp->sc_mask, &mask);
#endif
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);

	mutex_exit(p->p_lock);

	/*
	 * Restore the user supplied information.
	 */
	frame->f_regs[SP] = scp->sc_sp;
	frame->f_regs[D0] = scp->sc_d0;
	frame->f_regs[D1] = scp->sc_d1;
	frame->f_regs[A0] = scp->sc_a0;
	frame->f_regs[A1] = scp->sc_a1;
	frame->f_pc = scp->sc_pc;
	/* Privileged bits of  sr  are silently ignored on Linux/m68k. */
	frame->f_sr = scp->sc_ps & ~(PSL_MBZ|PSL_IPL|PSL_S);
	/*
	 * Other registers are assumed to be unchanged,
	 * and not restored.
	 */

	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the sigcontext structure.
	 */
	if (scp->sc_ss.ss_format) {
		frame->f_format = scp->sc_ss.ss_format;
		frame->f_vector = scp->sc_ss.ss_vector;
		if (frame->f_stackadj < sz)	/* just in case... */
			goto bad;
		frame->f_stackadj -= sz;
		memcpy( &frame->F_u, &scp->sc_ss.ss_frame, sz);
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("linux_sys_sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, scp->sc_ss.ss_format);
#endif
	}

	/*
	 * Finally we restore the original FP context.
	 */
	switch (fputype) {
	case FPU_NONE:
		break;
#ifdef M68060
	case FPU_68060:
		if (((struct fpframe060*)&scp->sc_ss.ss_fpstate.FPF_u1)
					->fpf6_frmfmt != FPF6_FMT_NULL) {
			/*
			 * On 060,  "fmovem <ea>,fpcr/fpsr/fpi"  is
			 * emulated by software and slow.
			 */
			__asm("fmovem %0,%%fpcr; fmovem %1,%%fpsr; fmovem %2,%%fpi"::
				"m" (scp->sc_ss.ss_fpstate.fpf_fpcr),
				"m" (scp->sc_ss.ss_fpstate.fpf_fpsr),
				"m" (scp->sc_ss.ss_fpstate.fpf_fpiar));
			__asm("fmovem %0,%%fp0-%%fp1" : :
				"m" (scp->sc_ss.ss_fpstate.fpf_regs[0][0]));
		}
		__asm("frestore %0" : : "m" (scp->sc_ss.ss_fpstate.FPF_u1));
		break;
#endif
	default:
		if (scp->sc_ss.ss_fpstate.fpf_version) {
			__asm("fmovem %0,%%fpcr/%%fpsr/%%fpi; fmovem %1,%%fp0-%%fp1"::
				"m" (scp->sc_ss.ss_fpstate.fpf_fpcr),
				"m" (scp->sc_ss.ss_fpstate.fpf_regs[0][0]));
		}
		__asm("frestore %0" : : "m" (scp->sc_ss.ss_fpstate.FPF_u1));
		break;
	}

#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&scp->sc_ss.ss_fpstate)
		printf("linux_sys_sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&scp->sc_ss.ss_fpstate,
		       &scp->sc_ss.ss_fpstate);
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("linux_sys_sigreturn(%d): returns\n", p->p_pid);
#endif

	return EJUSTRETURN;
}

/* ARGSUSED */
int
linux_sys_rt_sigreturn(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct frame *frame;
	struct linux_ucontext *ucp;	/* ucontext in user space */
	struct linux_ucontext tuc;	/* copy of *ucp */
	sigset_t mask;
	int sz = 0, error;		/* extra frame size */

	/*
	 * rt_sigreturn of Linux/m68k takes no arguments.
	 * usp + 4 is a pointer to siginfo structure,
	 * usp + 8 is a pointer to ucontext structure.
	 */
	frame = (struct frame *) l->l_md.md_regs;
	error = copyin((char *)frame->f_regs[SP] + 8, (void *)&ucp,
	    sizeof(void *));
	if (error || (int) ucp & 1)
		goto bad;		/* error or odd address */

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("linux_rt_sigreturn: pid %d, ucp %p\n", p->p_pid, ucp);
#endif

	/* Grab whole of the ucontext. */
	if (copyin(ucp, &tuc, sizeof tuc)) {
bad:		
		mutex_enter(p->p_lock);
		sigexit(l, SIGSEGV);
	}

	/*
	 * Check kernel stack and re-enter to syscall() if needed.
	 */
	if ((sz = tuc.uc_ss.ss_format) != 0) {
		if ((sz = exframesize[sz]) < 0)
			goto bad;
		if (sz && frame->f_stackadj == 0) {
			/*
			 * Extra stack space is required but not allocated.
			 * Allocate and re-enter syscall().
			 */
			reenter_syscall(frame, sz);
			/* NOTREACHED */
		}
	}
#ifdef DEBUG
	/* reenter_syscall() doesn't adjust stack. */
	if (sz != frame->f_stackadj)
		panic("linux_sys_rt_sigreturn: adj: %d != %d",
			sz, frame->f_stackadj);
#endif

	if (tuc.uc_mc.mc_version != LINUX_MCONTEXT_VERSION)
		goto bad;

	mutex_enter(p->p_lock);

	/* Restore signal stack. */
	l->l_sigstk.ss_flags =
		(l->l_sigstk.ss_flags & ~SS_ONSTACK) |
		(tuc.uc_stack.ss_flags & LINUX_SS_ONSTACK ? SS_ONSTACK : 0);

	/* Restore signal mask. */
	linux_to_native_sigset(&mask, &tuc.uc_sigmask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);

	mutex_exit(p->p_lock);

	/*
	 * Restore the user supplied information.
	 */
	memcpy( frame->f_regs, tuc.uc_mc.mc_gregs.gr_regs, sizeof(u_int)*16);
	frame->f_pc = tuc.uc_mc.mc_gregs.gr_pc;
	/* Privileged bits of  sr  are silently ignored on Linux/m68k. */
	frame->f_sr = tuc.uc_mc.mc_gregs.gr_sr & ~(PSL_MBZ|PSL_IPL|PSL_S);

	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the ucontext structure.
	 */
	if (tuc.uc_ss.ss_format) {
		frame->f_format = tuc.uc_ss.ss_format;
		frame->f_vector = tuc.uc_ss.ss_vector;
		if (frame->f_stackadj < sz)	/* just in case... */
			goto bad;
		frame->f_stackadj -= sz;
		memcpy( &frame->F_u, &tuc.uc_ss.ss_frame, sz);
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("linux_sys_rt_sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, tuc.uc_ss.ss_format);
#endif
	}

	/*
	 * Finally we restore the original FP context.
	 */
	switch (fputype) {
	case FPU_NONE:
		break;
#ifdef M68060
	case FPU_68060:
		if (((struct fpframe060*)&tuc.uc_ss.ss_fpstate.FPF_u1)
					->fpf6_frmfmt != FPF6_FMT_NULL) {
			/*
			 * On 060,  "fmovem <ea>,fpcr/fpsr/fpi"  is
			 * emulated by software and slow.
			 */
			__asm("fmovem %0,%%fpcr; fmovem %1,%%fpsr; fmovem %2,%%fpi"::
				"m" (tuc.uc_mc.mc_fpregs.fpr_fpcr),
				"m" (tuc.uc_mc.mc_fpregs.fpr_fpsr),
				"m" (tuc.uc_mc.mc_fpregs.fpr_fpiar));
			__asm("fmovem %0,%%fp0-%%fp1" : :
				"m" (tuc.uc_mc.mc_fpregs.fpr_regs[0][0]));
		}
		__asm("frestore %0" : : "m" (tuc.uc_ss.ss_fpstate.FPF_u1));
		break;
#endif
	default:
		if (tuc.uc_ss.ss_fpstate.fpf_version) {
			__asm("fmovem %0,%%fpcr/%%fpsr/%%fpi; fmovem %1,%%fp0-%%fp1"::
				"m" (tuc.uc_mc.mc_fpregs.fpr_fpcr),
				"m" (tuc.uc_mc.mc_fpregs.fpr_regs[0][0]));
		}
		__asm("frestore %0" : : "m" (tuc.uc_ss.ss_fpstate.FPF_u1));
		break;
	}

#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tuc.uc_ss.ss_fpstate)
		printf("linux_rt_sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&tuc.uc_ss.ss_fpstate,
		       &tuc.uc_ss.ss_fpstate);
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("linux_rt_sigreturn(%d): returns\n", p->p_pid);
#endif

	return EJUSTRETURN;
}

/*
 * MPU cache operation of Linux/m68k,
 * mainly used for dynamic linking.
 */

/* scope */
#define LINUX_FLUSH_SCOPE_LINE	1	/* a cache line */
#define LINUX_FLUSH_SCOPE_PAGE	2	/* a page */
#define LINUX_FLUSH_SCOPE_ALL	3	/* the whole cache */
/* cache */
#define LINUX_FLUSH_CACHE_DATA	1	/* flush and purge data cache */
#define LINUX_FLUSH_CACHE_INSN	2	/* purge instruction cache */
#define LINUX_FLUSH_CACHE_BOTH	3	/* both */

/* ARGSUSED */
int
linux_sys_cacheflush(struct lwp *l, const struct linux_sys_cacheflush_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned long)	addr;
		syscallarg(int)			scope;
		syscallarg(int)			cache;
		syscallarg(unsigned long)	len;
	} */
	struct proc *p = l->l_proc;
	int scope, cache;
	vaddr_t addr;
	int len;
	int error;

	scope = SCARG(uap, scope);
	cache = SCARG(uap, cache);

	if (scope < LINUX_FLUSH_SCOPE_LINE || scope > LINUX_FLUSH_SCOPE_ALL
				|| cache & ~LINUX_FLUSH_CACHE_BOTH)
		return EINVAL;

#if defined(M68040) || defined(M68060)
	addr = (vaddr_t) SCARG(uap, addr);
	len = (int) SCARG(uap, len);
#else
	/*
	 * We always flush entire cache on 68020/030
	 * and these values are not used afterwards.
	 */
	addr = 0;
	len = 0;
#endif

	/*
	 * LINUX_FLUSH_SCOPE_ALL (flush whole cache) is limited to super users.
	 */
	if (scope == LINUX_FLUSH_SCOPE_ALL) {
		if ((error = kauth_authorize_machdep(l->l_cred,
		    KAUTH_MACHDEP_CACHEFLUSH, NULL, NULL, NULL, NULL)) != 0)
			return error;
#if defined(M68040) || defined(M68060)
		/* entire cache */
		len = INT_MAX;
#endif
	}

	error = 0;
	if (cache & LINUX_FLUSH_CACHE_DATA)
		if ((error = cachectl1(CC_EXTPURGE|CC_PURGE, addr, len, p)) !=0)
			return error;
	if (cache & LINUX_FLUSH_CACHE_INSN)
		error = cachectl1(CC_EXTPURGE|CC_IPURGE, addr, len, p);

	return error;
}

/*
 * Convert NetBSD's devices to Linux's.
 */
dev_t
linux_fakedev(dev_t dev, int raw)
{

	/* do nothing for now */
	return dev;
}

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call.
 */
int
linux_machdepioctl(struct lwp *l, const struct linux_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
	struct sys_ioctl_args bia;
	u_long com;

	SCARG(&bia, fd) = SCARG(uap, fd);
	SCARG(&bia, data) = SCARG(uap, data);
	com = SCARG(uap, com);

	switch (com) {

	/* do nothing for now */

	default:
		printf("linux_machdepioctl: invalid ioctl %08lx\n", com);
		return EINVAL;
	}
	SCARG(&bia, com) = com;
	return sys_ioctl(l, &bia, retval);
}

int
linux_usertrap(struct lwp *l, vaddr_t trapaddr, void *arg)
{
	return 0;
}
