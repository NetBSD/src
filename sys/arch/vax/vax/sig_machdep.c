/* $NetBSD: sig_machdep.c,v 1.11.4.1 2007/07/11 20:02:59 mjf Exp $	 */

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

/*
 * Copyright (c) 2002, Hugh Graham.
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
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
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sig_machdep.c,v 1.11.4.1 2007/07/11 20:02:59 mjf Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/kernel.h>
#include <sys/msgbuf.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/ptrace.h>
#include <sys/ksyms.h>

#include <dev/cons.h>

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#endif

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <machine/sid.h>
#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/cpu.h>
#include <machine/macros.h>
#include <machine/nexus.h>
#include <machine/trap.h>
#include <machine/reg.h>
#include <machine/db_machdep.h>
#include <machine/scb.h>
#include <vax/vax/gencons.h>

#ifdef DDB
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif
typedef vaddr_t (*sig_setupstack_t)(const ksiginfo_t *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)
static vaddr_t setupstack_oldsigcontext(const ksiginfo_t *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);
#endif
#if defined(COMPAT_16) || defined(COMPAT_ULTRIX)
static vaddr_t setupstack_sigcontext2(const ksiginfo_t *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);
#endif
static vaddr_t setupstack_siginfo3(const ksiginfo_t *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);

const static sig_setupstack_t sig_setupstacks[] = {
#if defined(COMPAT_13) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)
	setupstack_oldsigcontext,	/* 0 */
	setupstack_oldsigcontext,	/* 1 */
#else
	0,				/* 0 */
	0,				/* 1 */
#endif
#if defined(COMPAT_16) || defined(COMPAT_ULTRIX)
	setupstack_sigcontext2,		/* 2 */
#else
	0,				/* 2 */
#endif
	setupstack_siginfo3,		/* 3 */
};

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)
int
compat_13_sys_sigreturn(struct lwp *l, void *v, register_t *retval)
{
	struct compat_13_sys_sigreturn_args /* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct trapframe *scf;
	struct sigcontext13 *ucntx;
	struct sigcontext13 ksc;
	sigset_t mask;

	scf = l->l_addr->u_pcb.framep;
	ucntx = SCARG(uap, sigcntxp);
	if (copyin((void *)ucntx, (void *)&ksc, sizeof(struct sigcontext)))
		return EINVAL;

	/* Compatibility mode? */
	if ((ksc.sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((ksc.sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (ksc.sc_ps & PSL_CM)) {
		return (EINVAL);
	}

	mutex_enter(&p->p_smutex);
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	native_sigset13_to_sigset(&ksc.sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(&p->p_smutex);

	scf->fp = ksc.sc_fp;
	scf->ap = ksc.sc_ap;
	scf->pc = ksc.sc_pc;
	scf->sp = ksc.sc_sp;
	scf->psl = ksc.sc_ps;
	return (EJUSTRETURN);
}

struct otrampframe {
	unsigned sig;	/* Signal number */
	unsigned code;	/* Info code */
	vaddr_t	scp;	/* Pointer to struct sigcontext */
	unsigned r0, r1, r2, r3, r4, r5; /* Registers saved when interrupt */
	register_t pc;	/* Address of signal handler */
	vaddr_t	arg;	/* Pointer to first (and only) sigreturn argument */
};

static vaddr_t
setupstack_oldsigcontext(const ksiginfo_t *ksi, const sigset_t *mask, int vers,
	struct lwp *l, struct trapframe *tf, vaddr_t sp, int onstack,
	vaddr_t handler)
{
	struct sigcontext sigctx;
	struct otrampframe tramp;
	struct proc *p = l->l_proc;
	bool error;

	sigctx.sc_pc = tf->pc;
	sigctx.sc_ps = tf->psl;
	sigctx.sc_ap = tf->ap;
	sigctx.sc_fp = tf->fp; 
	sigctx.sc_sp = tf->sp; 
	sigctx.sc_onstack = onstack ? SS_ONSTACK : 0;
	sigctx.sc_mask = *mask;
	sp -= sizeof(struct sigcontext);

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
	native_sigset_to_sigset13(mask, &sigctx.__sc_mask13);
#endif

	tramp.sig = ksi->ksi_signo;
	tramp.code = (register_t)ksi->ksi_addr;
	/* Set up positions for structs on stack */
	tramp.scp = sp;
	/* r0..r5 are saved by the popr in the sigcode snippet but we need
	   to zero them anyway.  */
	tramp.r0 = tramp.r1 = tramp.r2 = tramp.r3 = tramp.r4 = tramp.r5 = 0;
	tramp.pc = (register_t)handler;
	tramp.arg = sp;
	sendsig_reset(l, ksi->ksi_signo);
	mutex_exit(&p->p_smutex);

	/* Point stack pointer at pc in trampoline.  */
	sp =- 8;

	error = copyout(&tramp, (char *)tramp.scp - sizeof(tramp), sizeof(tramp)) != 0 ||
	    copyout(&sigctx, (void *)tramp.scp, sizeof(sigctx)) != 0;

	mutex_enter(&p->p_smutex);
	if (error)
		return 0;

	return sp;
}
#endif /* COMPAT_13 || COMPAT_ULTRIX || COMPAT_IBCS2 */

#if defined(COMPAT_16) || defined(COMPAT_ULTRIX)
int
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct trapframe *scf;
	struct sigcontext *ucntx;
	struct sigcontext ksc;

	scf = l->l_addr->u_pcb.framep;
	ucntx = SCARG(uap, sigcntxp);

	if (copyin((void *)ucntx, (void *)&ksc, sizeof(struct sigcontext)))
		return EINVAL;
	/* Compatibility mode? */
	if ((ksc.sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((ksc.sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (ksc.sc_ps & PSL_CM)) {
		return (EINVAL);
	}

	mutex_enter(&p->p_smutex);
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &ksc.sc_mask, 0);
	mutex_exit(&p->p_smutex);

	scf->fp = ksc.sc_fp;
	scf->ap = ksc.sc_ap;
	scf->pc = ksc.sc_pc;
	scf->sp = ksc.sc_sp;
	scf->psl = ksc.sc_ps;
	return (EJUSTRETURN);
}

/*
 * Brief description of how sendsig() works:
 * A struct sigcontext is allocated on the user stack. The relevant
 * registers are saved in it. Below it is a struct trampframe constructed, it
 * is actually an argument list for callg. The user
 * stack pointer is put below all structs.
 *
 * The registers will contain when the signal handler is called:
 * pc, psl	- Obvious
 * sp		- An address below all structs
 * fp 		- The address of the signal handler
 * ap		- The address to the callg frame
 *
 * The trampoline code will save r0-r5 before doing anything else.
 */
struct trampoline2 {
	unsigned int narg;	/* Argument count (== 3) */
	unsigned int sig;	/* Signal number */
	unsigned int code;	/* Info code */
	vaddr_t scp;		/* Pointer to struct sigcontext */
};


static vaddr_t
setupstack_sigcontext2(const ksiginfo_t *ksi, const sigset_t *mask, int vers,
	struct lwp *l, struct trapframe *tf, vaddr_t sp, int onstack,
	vaddr_t handler)
{
	struct trampoline2 tramp;
	struct sigcontext sigctx;
	struct proc *p = l->l_proc;
	bool error;

	/* The sigcontext struct will be passed back to sigreturn().  */
	sigctx.sc_pc = tf->pc;
	sigctx.sc_ps = tf->psl;
	sigctx.sc_ap = tf->ap;
	sigctx.sc_fp = tf->fp;
	sigctx.sc_sp = tf->sp;
	sigctx.sc_onstack = onstack ? SS_ONSTACK : 0;
	sigctx.sc_mask = *mask;
	sp -= sizeof(struct sigcontext);

	/* Arguments given to the signal handler.  */
	tramp.narg = 3;
	tramp.sig = ksi->ksi_signo;
	tramp.code = (register_t)ksi->ksi_addr;
	tramp.scp = sp;
	sp -= sizeof(tramp);
	sendsig_reset(l, ksi->ksi_signo);
	mutex_exit(&p->p_smutex);

	/* Store the handler in the trapframe.  */
	tf->fp = handler;

	/* Copy out the sigcontext and trampoline.  */
	error = (copyout(&sigctx, (char *)tramp.scp, sizeof(sigctx)) != 0 ||
	    copyout(&tramp, (char *)sp, sizeof(tramp)) != 0);

	mutex_enter(&p->p_smutex);
	if (error)
		return 0;

	/* return updated stack pointer */
	return sp;
}
#endif	/* COMPAT_16 || COMPAT_ULTRIX */

/*
 * Brief description of how sendsig() works:
 * A struct sigcontext is allocated on the user stack. The relevant
 * registers are saved in it. Below it is a struct trampframe constructed, it
 * is actually an argument list for callg. The user
 * stack pointer is put below all structs.
 *
 * The registers will contain when the signal handler is called:
 * pc, psl	- Obvious
 * sp		- An address below all structs
 * fp 		- The address of the signal handler
 * ap		- The address to the callg frame
 *
 * The trampoline code will save r0-r5 before doing anything else.
 */
struct trampoline3 {
	unsigned int narg;	/* Argument count (== 3) */
	int sig;		/* Signal number */
	vaddr_t sip;		/* Pointer to siginfo_t */
	vaddr_t ucp;		/* Pointer to ucontext_t */
};

static vaddr_t
setupstack_siginfo3(const ksiginfo_t *ksi, const sigset_t *mask, int vers,
	struct lwp *l, struct trapframe *tf, vaddr_t sp, int onstack,
	vaddr_t handler)
{
	struct trampoline3 tramp;
	struct proc *p = l->l_proc;
	ucontext_t uc;
	bool error;

	/*
	 * Arguments given to the signal handler.
	 */
	tramp.narg = 3;
	tramp.sig = ksi->ksi_signo;
	sp -= sizeof(uc);		tramp.ucp = sp;
	sp -= sizeof(siginfo_t);	tramp.sip = sp;
	sp -= sizeof(tramp);

	/* Save register context.  */
	uc.uc_flags = _UC_SIGMASK;
	uc.uc_sigmask = *mask;
	uc.uc_link = l->l_ctxlink;
	memset(&uc.uc_stack, 0, sizeof(uc.uc_stack));
	sendsig_reset(l, ksi->ksi_signo);
	mutex_exit(&p->p_smutex);
	cpu_getmcontext(l, &uc.uc_mcontext, &uc.uc_flags);

	tf->fp = handler;

	/* Copy the context to the stack.  */
	error = (copyout(&uc, (char *)tramp.ucp, sizeof(uc)) != 0 ||
	    copyout(&ksi->ksi_info, (char *)tramp.sip, sizeof(ksi->ksi_info)) != 0 ||
	    copyout(&tramp, (char *)sp, sizeof(tramp)) != 0);

	mutex_enter(&p->p_smutex);
	if (error)
		sigexit(l, SIGILL);

	return sp;
};

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_addr->u_pcb.framep;
	struct sigaltstack *ss = &l->l_sigstk;
	const struct sigact_sigdesc *sd =
	    &p->p_sigacts->sa_sigdesc[ksi->ksi_signo];
	vaddr_t sp;
	int onstack;
	sig_setupstack_t setup;

	/* Figure what stack we are running on.  */
	onstack = (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (sd->sd_sigact.sa_flags & SA_ONSTACK) != 0;
	sp = onstack ? ((vaddr_t)ss->ss_sp + ss->ss_size) : tf->sp;

	if (sd->sd_vers > 3 || (setup = sig_setupstacks[sd->sd_vers]) == NULL)
		goto nosupport;

	sp = (*setup)(ksi, mask, sd->sd_vers, l, tf, sp, onstack,
	    (vaddr_t)sd->sd_sigact.sa_handler);
	if (sp == 0)
		goto nosupport;

	if (sd->sd_vers == 0)
		tf->pc = (register_t)p->p_sigctx.ps_sigcode;
	else
		tf->pc = (register_t)sd->sd_tramp;

	tf->psl = PSL_U | PSL_PREVU;
	tf->sp = sp;
	tf->ap = sp;

	if (onstack)
		ss->ss_flags |= SS_ONSTACK;
	return;

  nosupport:
	/* Don't know what trampoline version; kill it. */
	printf("sendsig(sig %d): bad version %d\n",
	    ksi->ksi_signo, sd->sd_vers);
	sigexit(l, SIGILL);
}
