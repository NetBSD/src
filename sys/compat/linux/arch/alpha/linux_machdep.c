/*	$NetBSD: linux_machdep.c,v 1.15.2.4 2001/10/08 20:10:47 nathanw Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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
 *
 * Based on sys/arch/i386/i386/linux_machdep.c:
 *	linux_machdep.c,v 1.42 1998/09/11 12:50:06 mycroft Exp
 *	written by Frank van der Linden
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/filedesc.h>
#include <sys/exec_elf.h>
#include <sys/ioctl.h>

#include <uvm/uvm_extern.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_emuldata.h>

#include <compat/linux/linux_syscallargs.h>

#include <machine/alpha.h>
#include <machine/reg.h>

#if defined(_KERNEL_OPT)
#include "wsdisplay.h"
#endif
#if (NWSDISPLAY >0)
#include <dev/wscons/wsdisplay_usl_io.h>
#endif
#ifdef DEBUG
#include <machine/sigdebug.h>
#endif

/*
 * Deal with some alpha-specific things in the Linux emulation code.
 */

void
linux_setregs(l, epp, stack)
	struct lwp *l;
	struct exec_package *epp;
	u_long stack;
{
#ifdef DEBUG
	struct trapframe *tfp = l->l_md.md_tf;
#endif

	setregs(l, epp, stack);
#ifdef DEBUG
	/*
	 * Linux has registers set to zero on entry; for DEBUG kernels
	 * the alpha setregs() fills registers with 0xbabefacedeadbeef.
	 */
	memset(tfp->tf_regs, 0, FRAME_SIZE * sizeof tfp->tf_regs[0]);
#endif
}

void setup_linux_rt_sigframe(tf, sig, mask)
	struct trapframe *tf;
	int sig;
	sigset_t *mask;
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct linux_rt_sigframe *sfp, sigframe;
	int onstack;
	int fsize, rndfsize;
	extern char linux_rt_sigcode[], linux_rt_esigcode[];

	/* Do we need to jump onto the signal stack? */
	onstack = (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
		  (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context.  */
	fsize = sizeof(struct linux_rt_sigframe);
	rndfsize = ((fsize + 15) / 16) * 16;

	if (onstack)
		sfp = (struct linux_rt_sigframe *)
					((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		sfp = (struct linux_rt_sigframe *)(alpha_pal_rdusp());
	sfp = (struct linux_rt_sigframe *)((caddr_t)sfp - rndfsize);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && (p->p_pid == sigpid))
		printf("linux_sendsig(%d): sig %d ssp %p usp %p\n", p->p_pid,
		    sig, &onstack, sfp);
#endif /* DEBUG */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	bzero(&sigframe.uc, sizeof(struct linux_ucontext));
	sigframe.uc.uc_mcontext.sc_onstack = onstack;

	/* Setup potentially partial signal mask in sc_mask. */
	/* But get all of it in uc_sigmask */
	native_to_linux_old_sigset(mask, &sigframe.uc.uc_mcontext.sc_mask);
	native_to_linux_sigset(mask, &sigframe.uc.uc_sigmask);

	sigframe.uc.uc_mcontext.sc_pc = tf->tf_regs[FRAME_PC];
	sigframe.uc.uc_mcontext.sc_ps = ALPHA_PSL_USERMODE;
	frametoreg(tf, (struct reg *)sigframe.uc.uc_mcontext.sc_regs);
	sigframe.uc.uc_mcontext.sc_regs[R_SP] = alpha_pal_rdusp();

	alpha_enable_fp(l, 1);
	sigframe.uc.uc_mcontext.sc_fpcr = alpha_read_fpcr();
	sigframe.uc.uc_mcontext.sc_fp_control = alpha_read_fp_c(l);
	alpha_pal_wrfen(0);

	sigframe.uc.uc_mcontext.sc_traparg_a0 = tf->tf_regs[FRAME_A0];
	sigframe.uc.uc_mcontext.sc_traparg_a1 = tf->tf_regs[FRAME_A1];
	sigframe.uc.uc_mcontext.sc_traparg_a2 = tf->tf_regs[FRAME_A2];

	/*
	 * XXX XAX Create bogus siginfo data.  This can't really
	 * XXX be fixed until NetBSD has realtime signals.
	 * XXX Or we do the emuldata thing.
	 * XXX -erh
	 */
	bzero(&sigframe.info, sizeof(struct linux_siginfo));
	sigframe.info.lsi_signo = sig;
	sigframe.info.lsi_code = LINUX_SI_USER;
	sigframe.info.lsi_pid = p->p_pid;
	sigframe.info.lsi_uid = p->p_ucred->cr_uid;	/* Use real uid here? */

	if (copyout((caddr_t)&sigframe, (caddr_t)sfp, fsize) != 0) {
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

	/* Pass pointers to siginfo and ucontext in the regs */
	tf->tf_regs[FRAME_A1] = (unsigned long)&sfp->info;
	tf->tf_regs[FRAME_A2] = (unsigned long)&sfp->uc;

	/* Address of trampoline code.  End up at this PC after mi_switch */
	tf->tf_regs[FRAME_PC] =
	    (u_int64_t)(PS_STRINGS - (linux_rt_esigcode - linux_rt_sigcode));

	/* Adjust the stack */
	alpha_pal_wrusp((unsigned long)sfp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

void setup_linux_sigframe(tf, sig, mask)
	struct trapframe *tf;
	int sig;
	sigset_t *mask;
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct linux_sigframe *sfp, sigframe;
	int onstack;
	int fsize, rndfsize;
	extern char linux_sigcode[], linux_esigcode[];

	/* Do we need to jump onto the signal stack? */
	onstack = (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
		  (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context.  */
	fsize = sizeof(struct linux_sigframe);
	rndfsize = ((fsize + 15) / 16) * 16;

	if (onstack)
		sfp = (struct linux_sigframe *)
					((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		sfp = (struct linux_sigframe *)(alpha_pal_rdusp());
	sfp = (struct linux_sigframe *)((caddr_t)sfp - rndfsize);

#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && (p->p_pid == sigpid))
		printf("linux_sendsig(%d): sig %d ssp %p usp %p\n", p->p_pid,
		    sig, &onstack, sfp);
#endif /* DEBUG */

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	bzero(&sigframe.sf_sc, sizeof(struct linux_ucontext));
	sigframe.sf_sc.sc_onstack = onstack;
	native_to_linux_old_sigset(mask, &sigframe.sf_sc.sc_mask);
	sigframe.sf_sc.sc_pc = tf->tf_regs[FRAME_PC];
	sigframe.sf_sc.sc_ps = ALPHA_PSL_USERMODE;
	frametoreg(tf, (struct reg *)sigframe.sf_sc.sc_regs);
	sigframe.sf_sc.sc_regs[R_SP] = alpha_pal_rdusp();

	if (l == fpcurproc) {
	    alpha_pal_wrfen(1);
	    savefpstate(&l->l_addr->u_pcb.pcb_fp);
	    alpha_pal_wrfen(0);
	    sigframe.sf_sc.sc_fpcr = l->l_addr->u_pcb.pcb_fp.fpr_cr;
	    fpcurproc = NULL;
	}
	/* XXX ownedfp ? etc...? */

	sigframe.sf_sc.sc_traparg_a0 = tf->tf_regs[FRAME_A0];
	sigframe.sf_sc.sc_traparg_a1 = tf->tf_regs[FRAME_A1];
	sigframe.sf_sc.sc_traparg_a2 = tf->tf_regs[FRAME_A2];

	if (copyout((caddr_t)&sigframe, (caddr_t)sfp, fsize) != 0) {
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

	/* Pass pointers to sigcontext in the regs */
	tf->tf_regs[FRAME_A1] = 0;
	tf->tf_regs[FRAME_A2] = (unsigned long)&sfp->sf_sc;

	/* Address of trampoline code.  End up at this PC after mi_switch */
	tf->tf_regs[FRAME_PC] =
	    (u_int64_t)(PS_STRINGS - (linux_esigcode - linux_sigcode));

	/* Adjust the stack */
	alpha_pal_wrusp((unsigned long)sfp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
linux_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curproc;
	struct trapframe *tf = l->l_md.md_tf;
#ifdef notyet
	struct linux_emuldata *edp;

	/* Setup the signal frame (and part of the trapframe) */
	/*OLD: if (p->p_sigacts->ps_siginfo & sigmask(sig))*/
/*	XXX XAX this is broken now.  need someplace to store what
	XXX XAX kind of signal handler a signal has.*/
#if 0
	edp = (struct linux_emuldata *)p->p_emuldata;
#else
	edp = 0;
#endif
	if (edp && sigismember(&edp->ps_siginfo, sig))
		setup_linux_rt_sigframe(tf, sig, mask);
	else
#endif /* notyet */
		setup_linux_sigframe(tf, sig, mask);

	/* Signal handler for trampoline code */
	tf->tf_regs[FRAME_T12] = (u_int64_t)catcher;
	tf->tf_regs[FRAME_A0] = native_to_linux_sig[sig];

	/*
	 * Linux has a custom restorer option.  To support it we would
	 * need to store an array of restorers and a sigcode block
	 * which knew to use it.  Doesn't seem worth the trouble.
	 * -erh
	 */

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig(%d): pc %lx, catcher %lx\n", l->l_proc->p_pid,
		    tf->tf_regs[FRAME_PC], tf->tf_regs[FRAME_A3]);
	if ((sigdebug & SDB_KSTACK) && l->l_proc->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n", l->l_proc->p_pid, sig);
#endif
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc as specified by context
 * left by sendsig.
 * Linux real-time signals use a different sigframe,
 * but the sigcontext is the same.
 */

int
linux_restore_sigcontext(struct lwp *l, struct linux_sigcontext context,
			 sigset_t *mask)
{

	struct proc *p = l->l_proc;
	/*
	 * Linux doesn't (yet) have alternate signal stacks.
	 * However, the OSF/1 sigcontext which they use has
	 * an onstack member.  This could be needed in the future.
	 */
	if (context.sc_onstack & LINUX_SA_ONSTACK)
	    p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
	    p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Reset the signal mask */
	(void) sigprocmask1(p, SIG_SETMASK, mask, 0);

	/*
	 * Check for security violations.
	 * Linux doesn't allow any changes to the PSL.
	 */
	if (context.sc_ps != ALPHA_PSL_USERMODE)
	    return(EINVAL);

	l->l_md.md_tf->tf_regs[FRAME_PC] = context.sc_pc;
	l->l_md.md_tf->tf_regs[FRAME_PS] = context.sc_ps;

	regtoframe((struct reg *)context.sc_regs, l->l_md.md_tf);
	alpha_pal_wrusp(context.sc_regs[R_SP]);

	if (l == fpcurproc)
	    fpcurproc = NULL;

	/* Restore fp regs and fpr_cr */
	bcopy((struct fpreg *)context.sc_fpregs, &l->l_addr->u_pcb.pcb_fp,
	    sizeof(struct fpreg));
	/* XXX sc_ownedfp ? */
	/* XXX sc_fp_control ? */

#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("linux_rt_sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}

int
linux_sys_rt_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigreturn_args /* {
		syscallarg(struct linux_rt_sigframe *) sfp;
	} */ *uap = v;
	struct linux_rt_sigframe *sfp, sigframe;
	sigset_t mask;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */

	sfp = SCARG(uap, sfp);

	if (ALIGN(sfp) != (u_int64_t)sfp)
		return(EINVAL);

	/*
	 * Fetch the frame structure.
	 */
	if (copyin((caddr_t)sfp, &sigframe,
			sizeof(struct linux_rt_sigframe)) != 0)
		return (EFAULT);

	/* Grab the signal mask */
	linux_to_native_sigset(&sigframe.uc.uc_sigmask, &mask);

	return(linux_restore_sigcontext(l, sigframe.uc.uc_mcontext, &mask));
}


int
linux_sys_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigreturn_args /* {
		syscallarg(struct linux_sigframe *) sfp;
	} */ *uap = v;
	struct linux_sigframe *sfp, frame;
	sigset_t mask;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */

	sfp = SCARG(uap, sfp);
	if (ALIGN(sfp) != (u_int64_t)sfp)
		return(EINVAL);

	/*
	 * Fetch the frame structure.
	 */
	if (copyin((caddr_t)sfp, &frame, sizeof(struct linux_sigframe)) != 0)
		return(EFAULT);

	/* Grab the signal mask. */
	/* XXX use frame.extramask */
	linux_old_to_native_sigset(frame.sf_sc.sc_mask, &mask);

	return(linux_restore_sigcontext(l, frame.sf_sc, &mask));
}

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call
 */
/* XXX XAX update this, add maps, etc... */
int
linux_machdepioctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	struct sys_ioctl_args bia;
	u_long com;

	SCARG(&bia, fd) = SCARG(uap, fd);
	SCARG(&bia, data) = SCARG(uap, data);
	com = SCARG(uap, com);

	switch (com) {
	default:
		printf("linux_machdepioctl: invalid ioctl %08lx\n", com);
		return EINVAL;
	}
	SCARG(&bia, com) = com;
	/* XXX njwlwp */
	return sys_ioctl(curproc, &bia, retval);
}

/* XXX XAX fix this */
dev_t
linux_fakedev(dev)
	dev_t dev;
{
	return dev;
}

