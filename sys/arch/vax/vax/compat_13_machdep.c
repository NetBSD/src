/*	$NetBSD: compat_13_machdep.c,v 1.4.12.1 2017/12/03 11:36:48 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_13_machdep.c,v 1.4.12.1 2017/12/03 11:36:48 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_ibcs2.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <dev/cons.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <machine/macros.h>
#include <machine/reg.h>
#include <vax/vax/gencons.h>

/*
 * This code was moved here from sys/arch/vax/vax/sig_machdep.c
 */

int
compat_13_sys_sigreturn(struct lwp *l, const struct compat_13_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct sigcontext13 *) sigcntxp;
	} */
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct sigcontext13 *ucntx;
	struct sigcontext13 ksc;
	sigset_t mask;

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

	native_sigset13_to_sigset(&ksc.sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	tf->tf_fp = ksc.sc_fp;
	tf->tf_ap = ksc.sc_ap;
	tf->tf_pc = ksc.sc_pc;
	tf->tf_sp = ksc.sc_sp;
	tf->tf_psl = ksc.sc_ps;
	return (EJUSTRETURN);
}

vaddr_t
setupstack_oldsigcontext(const struct ksiginfo *ksi, const sigset_t *mask,
	int vers, struct lwp *l, struct trapframe *tf, vaddr_t sp, int onstack,
	vaddr_t handler)
{
	struct sigcontext sigctx;
	struct otrampframe tramp;
	struct proc * const p = l->l_proc;
	bool error;

	sigctx.sc_pc = tf->tf_pc;
	sigctx.sc_ps = tf->tf_psl;
	sigctx.sc_ap = tf->tf_ap;
	sigctx.sc_fp = tf->tf_fp;
	sigctx.sc_sp = tf->tf_sp;
	sigctx.sc_onstack = onstack ? SS_ONSTACK : 0;
	sigctx.sc_mask = *mask;
	sp -= sizeof(struct sigcontext);

	native_sigset_to_sigset13(mask, &sigctx.__sc_mask13);

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
	mutex_exit(p->p_lock);

	/* Point stack pointer at pc in trampoline.  */
	sp =- 8;

	error = copyout(&tramp, (char *)tramp.scp - sizeof(tramp), sizeof(tramp)) != 0 ||
	    copyout(&sigctx, (void *)tramp.scp, sizeof(sigctx)) != 0;

	mutex_enter(p->p_lock);
	if (error)
		return 0;

	return sp;
}
