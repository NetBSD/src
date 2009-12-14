/*	$NetBSD: compat_13_machdep.c,v 1.18 2009/12/14 00:46:05 matt Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.18 2009/12/14 00:46:05 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <mips/regnum.h>

#ifdef DEBUG
extern int sigdebug;
/* XXX defined in mips_machdep.c */
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

#if !defined(__mips_o32)
#define	fpreg		fpreg_oabi
#endif

int
compat_13_sys_sigreturn(struct lwp *l, const struct compat_13_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */
	struct sigcontext13 *scp, ksc;
	struct proc *p = l->l_proc;
	int error;
	struct frame *f;
	sigset_t mask;

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
		printf("sigreturn13: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if ((error = copyin(scp, &ksc, sizeof(ksc))) != 0)
		return (error);

	if ((uint32_t)ksc.sc_regs[_R_ZERO] != 0xacedbadeU)/* magic number */
		return (EINVAL);

	/* Resture the register context. */
	f = l->l_md.md_regs;
	f->f_regs[_R_PC] = ksc.sc_pc;
	f->f_regs[_R_MULLO] = ksc.mullo;
	f->f_regs[_R_MULHI] = ksc.mulhi;
#if defined(__mips_o32)
	memcpy(&f->f_regs[1], &scp->sc_regs[1],
	    sizeof(scp->sc_regs) - sizeof(scp->sc_regs[0]));
#else
	for (size_t i = 1; i < __arraycount(scp->sc_regs); i++)
		f->f_regs[i] = scp->sc_regs[i];
#endif
	if (scp->sc_fpused) {
		struct pcb * const pcb = lwp_getpcb(l);
		*(struct fpreg *)&pcb->pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;
	}

	mutex_enter(p->p_lock);

	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	mutex_exit(p->p_lock);

	/* Restore signal mask-> */
	native_sigset13_to_sigset(&ksc.sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);

	return (EJUSTRETURN);
}
