/*	$NetBSD: linux_machdep.c,v 1.32 2014/11/09 17:48:07 maxv Exp $	*/

/*-
 * Copyright (c) 1995, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.32 2014/11/09 17:48:07 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/filedesc.h>
#include <sys/exec_elf.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>

#include <miscfs/specfs/specdev.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscallargs.h>

#include <arm/locore.h>

void
linux_setregs(struct lwp *l, struct exec_package *epp, vaddr_t stack)
{

	setregs(l, epp, stack);
}

void
linux_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = lwp_trapframe(l);
	struct linux_sigframe *fp, frame;
	int onstack, error;
	const int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;


	/*
	 * The Linux version of this code is in
	 * linux/arch/arm/kernel/signal.c.
	 */

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct linux_sigframe *)((char *)l->l_sigstk.ss_sp +
					  l->l_sigstk.ss_size);
	else
		fp = (struct linux_sigframe *)tf->tf_usr_sp;
	fp--;

	/* Build stack frame for signal trampoline. */

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
	frame.sf_sc.sc_sp     = tf->tf_usr_sp;
	frame.sf_sc.sc_lr     = tf->tf_usr_lr;
	frame.sf_sc.sc_pc     = tf->tf_pc;
	frame.sf_sc.sc_cpsr   = tf->tf_spsr;

	/* Save signal stack. */
	/* Linux doesn't save the onstack flag in sigframe */

	/* Save signal mask. */
	native_to_linux_old_extra_sigset(&frame.sf_sc.sc_mask,
	    frame.sf_extramask, mask);

	/* Other state (mostly faked) */
	/*
	 * trapno should indicate the trap that caused the signal:
	 * 6  -> undefined instruction
	 * 11 -> address exception
	 * 14 -> data/prefetch abort
	 */
	frame.sf_sc.sc_trapno = 0;
	frame.sf_sc.sc_error_code = 0;
	frame.sf_sc.sc_fault_address = (u_int32_t) ksi->ksi_addr;
	sendsig_reset(l, sig);

	mutex_exit(p->p_lock);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(p->p_lock);
	
	if (error != 0) {
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
	tf->tf_r0 = native_to_linux_signo[sig];
	tf->tf_r1 = 0; /* XXX Should be a siginfo_t */
	tf->tf_r2 = 0;
	tf->tf_r3 = (register_t)catcher;
	tf->tf_usr_sp = (register_t)fp;
	tf->tf_pc = (register_t)p->p_sigctx.ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

}

#if 0
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
linux_sys_rt_sigreturn(struct proc *p, void *v, register_t *retval)
{
	/* XXX XAX write me */
	return(ENOSYS);
}
#endif

int
linux_sys_sigreturn(struct lwp *l, const struct linux_sys_sigreturn_args *v,
	register_t *retval)
{
	struct trapframe * const tf = lwp_trapframe(l);
	struct proc * const p = l->l_proc;
	struct linux_sigframe *sfp, frame;
	sigset_t mask;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	sfp = (struct linux_sigframe *)tf->tf_usr_sp;
	if (copyin((void *)sfp, &frame, sizeof(*sfp)) != 0)
		return (EFAULT);

	/*
	 * Make sure the processor mode has not been tampered with and
	 * interrupts have not been disabled.
	 */
	if (!VALID_R15_PSR(frame.sf_sc.sc_pc, frame.sf_sc.sc_cpsr))
		return EINVAL;

	/* Restore register context. */
	tf->tf_r0    = frame.sf_sc.sc_r0;
	tf->tf_r1    = frame.sf_sc.sc_r1;
	tf->tf_r2    = frame.sf_sc.sc_r2;
	tf->tf_r3    = frame.sf_sc.sc_r3;
	tf->tf_r4    = frame.sf_sc.sc_r4;
	tf->tf_r5    = frame.sf_sc.sc_r5;
	tf->tf_r6    = frame.sf_sc.sc_r6;
	tf->tf_r7    = frame.sf_sc.sc_r7;
	tf->tf_r8    = frame.sf_sc.sc_r8;
	tf->tf_r9    = frame.sf_sc.sc_r9;
	tf->tf_r10   = frame.sf_sc.sc_r10;
	tf->tf_r11   = frame.sf_sc.sc_r11;
	tf->tf_r12   = frame.sf_sc.sc_r12;
	tf->tf_usr_sp = frame.sf_sc.sc_sp;
	tf->tf_usr_lr = frame.sf_sc.sc_lr;
	tf->tf_pc    = frame.sf_sc.sc_pc;
	tf->tf_spsr  = frame.sf_sc.sc_cpsr;

	mutex_enter(p->p_lock);

	/* Restore signal stack. */
	l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux_old_extra_to_native_sigset(&mask, &frame.sf_sc.sc_mask,
	    frame.sf_extramask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);

	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}

/*
 * major device numbers remapping
 */
dev_t
linux_fakedev(dev_t dev, int raw)
{
	/* XXX write me */
	return dev;
}

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call
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
