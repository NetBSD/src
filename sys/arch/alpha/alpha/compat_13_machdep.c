/* $NetBSD: compat_13_machdep.c,v 1.20.20.1 2014/08/10 06:53:49 tls Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.20.20.1 2014/08/10 06:53:49 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/alpha.h>

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
compat_13_sys_sigreturn(struct lwp *l, const struct compat_13_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */
	struct sigcontext13 *scp, ksc;
	struct proc *p = l->l_proc;
	struct pcb *pcb;
	sigset13_t mask13;
	sigset_t mask;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (ALIGN(scp) != (uint64_t)scp)
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
	pcb = lwp_getpcb(l);
	fpu_discard(true);
	memcpy(&pcb->pcb_fp, (struct fpreg *)ksc.sc_fpregs,
	    sizeof(struct fpreg));
	/* XXX ksc.sc_fp_control ? */

	mutex_enter(p->p_lock);
	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/*
	 * Restore signal mask.  Note the mask is a "long" in the stack
	 * frame.
	 */
	mask13 = ksc.sc_mask;
	native_sigset13_to_sigset(&mask13, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}
