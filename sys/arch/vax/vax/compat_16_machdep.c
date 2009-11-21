/*	$NetBSD: compat_16_machdep.c,v 1.2 2009/11/21 04:45:39 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.2 2009/11/21 04:45:39 rmind Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_ibcs2.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/proc.h>
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

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <machine/sid.h>
#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/cpu.h>
#include <machine/macros.h>
#include <machine/trap.h>
#include <machine/reg.h>
#include <machine/db_machdep.h>
#include <machine/scb.h>
#include <machine/signal.h>
#include <vax/vax/gencons.h>

/*
 * This code was moved here from sys/arch/vax/vax/sig_machdep.c
 */

int
compat_16_sys___sigreturn14(struct lwp *l, const struct compat_16_sys___sigreturn14_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */
	struct proc *p = l->l_proc;
	struct pcb *pcb = lwp_getpcb(l);
	struct trapframe *scf;
	struct sigcontext *ucntx;
	struct sigcontext ksc;

	scf = pcb->framep;
	ucntx = SCARG(uap, sigcntxp);

	if (copyin((void *)ucntx, (void *)&ksc, sizeof(struct sigcontext)))
		return EINVAL;
	/* Compatibility mode? */
	if ((ksc.sc_ps & (PSL_IPL | PSL_IS)) ||
	    ((ksc.sc_ps & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (ksc.sc_ps & PSL_CM)) {
		return (EINVAL);
	}

	mutex_enter(p->p_lock);
	if (ksc.sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask. */
	(void) sigprocmask1(l, SIG_SETMASK, &ksc.sc_mask, 0);
	mutex_exit(p->p_lock);

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
vaddr_t
setupstack_sigcontext2(const struct ksiginfo *ksi, const sigset_t *mask,
	int vers, struct lwp *l, struct trapframe *tf, vaddr_t sp, int onstack,
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
	mutex_exit(p->p_lock);

	/* Store the handler in the trapframe.  */
	tf->fp = handler;

	/* Copy out the sigcontext and trampoline.  */
	error = (copyout(&sigctx, (char *)tramp.scp, sizeof(sigctx)) != 0 ||
	    copyout(&tramp, (char *)sp, sizeof(tramp)) != 0);

	mutex_enter(p->p_lock);
	if (error)
		return 0;

	/* return updated stack pointer */
	return sp;
}
