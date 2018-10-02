/*	$NetBSD: netbsd32_machdep_13.c,v 1.1.2.2 2018/10/02 22:00:15 pgoyette Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep_13.c,v 1.1.2.2 2018/10/02 22:00:15 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_modular.h"
#include "opt_execfmt.h"
#include "firm_events.h"
#endif

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/core.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/socketvar.h>
#include <sys/ucontext.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>

#include <dev/sun/event_var.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_exec.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#include <compat/sys/siginfo.h>
#include <compat/sys/ucontext.h>

#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/vuid_event.h>
#include <machine/netbsd32_machdep.h>
#include <machine/userret.h>

int
compat_13_netbsd32_sigreturn(struct lwp *l, const struct compat_13_netbsd32_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct netbsd32_sigcontext13 *) sigcntxp;
	} */
	struct netbsd32_sigcontext13 *scp;
	struct netbsd32_sigcontext13 sc;
	struct trapframe64 *tf;
	struct proc *p = l->l_proc;
	sigset_t mask;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(l)) {
#ifdef DEBUG
		printf("%s: rwindow_save(%p) failed, sending SIGILL\n",
		    __func__, p);
		Debugger();
#endif
		mutex_enter(p->p_lock);
		sigexit(l, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("%s: %s[%d], sigcntxp %p\n", __func__,
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = (struct netbsd32_sigcontext13 *)(u_long)SCARG(uap, sigcntxp);
 	if ((vaddr_t)scp & 3 || (copyin((void *)scp, &sc, sizeof sc) != 0))
	{
#ifdef DEBUG
		printf("%s: copyin failed\n", __func__);
		Debugger();
#endif
		return (EINVAL);
	}
	scp = &sc;

	tf = l->l_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((sc.sc_pc | sc.sc_npc) & 3) != 0)
#ifdef DEBUG
	{
		printf("%s: pc %p or npc %p invalid\n",
		   __func__, sc.sc_pc, sc.sc_npc);
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(sc.sc_psr);
	tf->tf_pc = (int64_t)sc.sc_pc;
	tf->tf_npc = (int64_t)sc.sc_npc;
	tf->tf_global[1] = (int64_t)sc.sc_g1;
	tf->tf_out[0] = (int64_t)sc.sc_o0;
	tf->tf_out[6] = (int64_t)sc.sc_sp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("%s: return trapframe pc=%p sp=%p tstate=%x\n", __func__,
		       (int)tf->tf_pc, (int)tf->tf_out[6], (int)tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	mutex_enter(p->p_lock);
	if (scp->sc_onstack & SS_ONSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	else
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	/* Restore signal mask */
	native_sigset13_to_sigset((sigset13_t *)&scp->sc_mask, &mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);
	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}

void  
netbsd32_machdep_md_13_init(void)
{

	/* Nothing to do */
}
                
void    
netbsd32_machdep_md_13_fini(void)   
{
 
	/* Nothing to do */
}
