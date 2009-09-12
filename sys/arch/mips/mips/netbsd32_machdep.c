/*	$NetBSD: netbsd32_machdep.c,v 1.1.2.1 2009/09/12 17:37:00 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep.c,v 1.1.2.1 2009/09/12 17:37:00 matt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_sa.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/exec.h>
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
#include <sys/user.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <mips/cpu.h>
#include <mips/cache.h>
#include <mips/sysarch.h>
#include <mips/cachectl.h>
#include <mips/locore.h>
#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/reg.h>

#include <uvm/uvm_extern.h>

char machine32[] = MACHINE;
char machine_arch32[] = MACHINE32_ARCH;

#if 0
cpu_coredump32
netbsd32_cpu_upcall
netbsd32_vm_default_addr
#endif

int
netbsd32_sysarch(struct lwp *l, const struct netbsd32_sysarch_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */
	struct proc *p = l->l_proc;
	void *parms = SCARG_P32(uap, parms);
	int error = 0;

	switch(SCARG(uap, op)) {
	case MIPS_CACHEFLUSH: {
		struct mips_cacheflush_args32 cfua;

		error = copyin(parms, &cfua, sizeof(cfua));
		if (error != 0)
			return (error);
		error =  mips_user_cacheflush(p, cfua.va, cfua.nbytes,
		     cfua.whichcache);
		break;
	}
	case MIPS_CACHECTL: {
		struct mips_cachectl_args32 ccua;

		error = copyin(parms, &ccua, sizeof(ccua));
		if (error != 0)
			return (error);
		error = mips_user_cachectl(p, ccua.va, ccua.nbytes, ccua.ctl);
		break;
	}
	default:
		error = ENOSYS;
		break;
	}
	return (error);
}

#ifdef COMPAT_16
int
compat_16_netbsd32___sigreturn14(struct lwp *l,
	const struct compat_16_netbsd32___sigreturn14_args *uap,
	register_t *retval)
{
	struct compat_16_sys___sigreturn14_args ua;

	NETBSD32TOP_UAP(sigcntxp, struct sigcontext *);

	return compat_16_sys___sigreturn14(l, &ua, retval);
}
#endif

vaddr_t
netbsd32_vm_default_addr(struct proc *p, vaddr_t base, vsize_t size)
{
	return VM_DEFAULT_ADDRESS32(base, size);
}


struct sigframe_siginfo32 {
	siginfo32_t sf_si;
	ucontext32_t sf_uc;
};

/*
 * Send a signal to process.
 */
static void
netbsd32_sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct sigacts * const ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo32 *sfp = getframe(l, sig, &onstack);
	struct sigframe_siginfo32 sf;
	struct frame * const tf = l->l_md.md_regs;
	size_t sfsz;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	sfp--;

	netbsd32_si_to_si32(&sf.sf_si, (const siginfo_t *)&ksi->ksi_info);

        /* Build stack frame for signal trampoline. */
        switch (ps->sa_sigdesc[sig].sd_vers) {
        case 0:         /* handled by sendsig_sigcontext */
        case 1:         /* handled by sendsig_sigcontext */
        default:        /* unknown version */
                printf("sendsig_siginfo: bad version %d\n",
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
	tf->f_regs[_R_A0] = sig;
	tf->f_regs[_R_A1] = (intptr_t)&sfp->sf_si;
	tf->f_regs[_R_A2] = (intptr_t)&sfp->sf_uc;

	tf->f_regs[_R_PC] = (intptr_t)catcher;
	tf->f_regs[_R_T9] = (intptr_t)catcher;
	tf->f_regs[_R_SP] = (intptr_t)sfp;
	tf->f_regs[_R_RA] = (intptr_t)ps->sa_sigdesc[sig].sd_tramp;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void    
netbsd32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{               
#ifdef COMPAT_16    
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else    
#endif  
		netbsd32_sendsig_siginfo(ksi, mask);
}       

void
cpu_getmcontext32(struct lwp *l, mcontext32_t *mc32, unsigned int *flagsp)
{
	mcontext_o32_t * const mco32 = (mcontext_o32_t *)mc32;
	mcontext_t mc;
	size_t i;

	if (l->l_proc->p_md.md_abi == _MIPS_BSD_API_N32) {
		cpu_getmcontext(l, (mcontext_t *)mc32, flagsp);
		return;
	}

	cpu_getmcontext(l, &mc, flagsp);
	for (i = 0; i < __arraycount(mc.__gregs); i++)
		mco32->__gregs[i] = mc.__gregs[i];
	if (*flagsp & _UC_FPU)
		memcpy(&mco32->__fpregs, &mc.__fpregs,
		    sizeof(struct fpreg_oabi));
}

int
cpu_setmcontext32(struct lwp *l, const mcontext32_t *mc32, unsigned int flags)
{
	const mcontext_o32_t * const mco32 = (const mcontext_o32_t *)mc32;
	mcontext_t mc;
	size_t i;

	if (l->l_proc->p_md.md_abi == _MIPS_BSD_API_N32)
		return cpu_setmcontext(l, (const mcontext_t *)mc32, flags);

	for (i = 0; i < __arraycount(mc.__gregs); i++)
		mc.__gregs[i] = mco32->__gregs[i];
	if (flags & _UC_FPU)
		memcpy(&mc.__fpregs, &mco32->__fpregs,
		    sizeof(struct fpreg_oabi));
	return cpu_setmcontext(l, &mc, flags);
}

#ifdef COREDUMP
/*
 * Dump the machine specific segment at the start of a core dump.
 */
int
cpu_coredump32(struct lwp *l, void *iocookie, struct core32 *chdr)
{
	int error;
	struct coreseg cseg;
	struct cpustate {
		struct frame frame;
		struct fpreg fpregs;
	} cpustate;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
		chdr->c_hdrsize = ALIGN(sizeof(struct core));
		chdr->c_seghdrsize = ALIGN(sizeof(struct coreseg));
		chdr->c_cpusize = sizeof(struct cpustate);
		chdr->c_nseg++;
		return 0;
	}

	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurlwp)
		savefpregs(l);
	cpustate.frame = *l->l_md.md_regs;
	cpustate.fpregs = l->l_addr->u_pcb.pcb_fpregs;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_MACHINE, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE, &cseg,
	    chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE, &cpustate,
	    chdr->c_cpusize);
}
#endif
