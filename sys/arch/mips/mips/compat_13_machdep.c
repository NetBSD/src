/*	$NetBSD: compat_13_machdep.c,v 1.8 2000/12/22 22:58:55 jdolecek Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.8 2000/12/22 22:58:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <mips/regnum.h>

#ifdef DEBUG
extern int sigdebug;
/* XXX defined in mips_machdep.c */
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

int
compat_13_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_13_sys_sigreturn_args /* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */ *uap = v;
	struct sigcontext13 *scp, ksc;
	int error;
	struct frame *f;
	sigset_t mask;

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

	if ((int)ksc.sc_regs[ZERO] != 0xACEDBADE)	/* magic number */
		return (EINVAL);

	/* Resture the register context. */
	f = (struct frame *)p->p_md.md_regs;
	f->f_regs[PC] = ksc.sc_pc;
	f->f_regs[MULLO] = ksc.mullo;
	f->f_regs[MULHI] = ksc.mulhi;
	memcpy(&f->f_regs[1], &scp->sc_regs[1],
	    sizeof(scp->sc_regs) - sizeof(scp->sc_regs[0]));
	if (scp->sc_fpused)
		p->p_addr->u_pcb.pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;

	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	native_sigset13_to_sigset(&ksc.sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);

	return (EJUSTRETURN);
}
