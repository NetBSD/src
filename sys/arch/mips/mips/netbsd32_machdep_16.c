/*	$NetBSD: netbsd32_machdep_16.c,v 1.1.2.3 2018/09/29 07:36:44 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep_16.c,v 1.1.2.3 2018/09/29 07:36:44 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/exec.h>
#include <sys/cpu.h>
#include <sys/core.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/module_hook.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <mips/cache.h>
#include <mips/sysarch.h>
#include <mips/cachectl.h>
#include <mips/locore.h>
#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/pcb.h>

#include <uvm/uvm_extern.h>

void netbsd32_sendsig_siginfo(const ksiginfo_t *, const sigset_t *);

int netbsd32_sendsig_16(const ksiginfo_t *, const sigset_t *);

extern struct netbsd32_sendsig_hook_t netbsd32_sendsig_hook;

int
compat_16_netbsd32___sigreturn14(struct lwp *l,
	const struct compat_16_netbsd32___sigreturn14_args *uap,
	register_t *retval)
{
	struct compat_16_sys___sigreturn14_args ua;

	NETBSD32TOP_UAP(sigcntxp, struct sigcontext *);

	return compat_16_sys___sigreturn14(l, &ua, retval);
}

struct sigframe_siginfo32 {
	siginfo32_t sf_si;
	ucontext32_t sf_uc;
};

/*
 * Send a signal to process.
 */
static void
netbsd32_sendsig_sigcontext(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct sigacts * const ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo32 *sfp = getframe(l, sig, &onstack);
	struct sigframe_siginfo32 sf;
	struct trapframe * const tf = l->l_md.md_utf;
	size_t sfsz;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	sfp--;

	netbsd32_si_to_si32(&sf.sf_si, (const siginfo_t *)&ksi->ksi_info);

        /* Build stack frame for signal trampoline. */
        switch (ps->sa_sigdesc[sig].sd_vers) {
        case 0:         /* handled by sendsig_sigcontext */
        case 1:         /* handled by sendsig_sigcontext */
        default:        /* unknown version */
                printf("%s: bad version %d\n", __func__,
                    ps->sa_sigdesc[sig].sd_vers);
                sigexit(l, SIGILL);
        case 2:
                break;
        }

	sf.sf_uc.uc_flags = _UC_SIGMASK
	    | ((l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK);
	sf.sf_uc.uc_sigmask = *mask;
	sf.sf_uc.uc_link = (intptr_t)l->l_ctxlink;
	memset(&sf.sf_uc.uc_stack, 0, sizeof(sf.sf_uc.uc_stack));
	sfsz = offsetof(struct sigframe_siginfo32, sf_uc.uc_mcontext);
	if (p->p_md.md_abi == _MIPS_BSD_API_O32)
		sfsz += sizeof(mcontext_o32_t);
	else
		sfsz += sizeof(mcontext32_t);
	sendsig_reset(l, sig);
	mutex_exit(p->p_lock);
	cpu_getmcontext32(l, &sf.sf_uc.uc_mcontext, &sf.sf_uc.uc_flags);
	error = copyout(&sf, sfp, sfsz);
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
	 * Set up the registers to directly invoke the signal
	 * handler.  The return address will be set up to point
	 * to the signal trampoline to bounce us back.
	 */
	tf->tf_regs[_R_A0] = sig;
	tf->tf_regs[_R_A1] = (intptr_t)&sfp->sf_si;
	tf->tf_regs[_R_A2] = (intptr_t)&sfp->sf_uc;

	tf->tf_regs[_R_PC] = (intptr_t)catcher;
	tf->tf_regs[_R_T9] = (intptr_t)catcher;
	tf->tf_regs[_R_SP] = (intptr_t)sfp;
	tf->tf_regs[_R_RA] = (intptr_t)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

int
netbsd32_sendsig_16(const ksiginfo_t *ksi, const sigset_t *mask)
{               
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		netbsd32_sendsig_sigcontext(ksi, mask);
	else    
		netbsd32_sendsig_siginfo(ksi, mask);
}       

MODULE_SET_HOOK(netbsd32_sendsig_hook, "nb32_16", netbsd32_sendsig_16); 
MODULE_UNSET_HOOK(netbsd32_sendsig_hook);

void    
netbsd32_machdep_md_init(void)
{       
                
	netbsd32_sendsig_hook_set();
}               
                
void            
netbsd32_machdep_md_fini(void)
{       

	netbsd32_sendsig_hook_unset();
}
