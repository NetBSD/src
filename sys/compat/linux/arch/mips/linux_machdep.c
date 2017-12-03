/*	$NetBSD: linux_machdep.c,v 1.42.14.1 2017/12/03 11:36:54 jdolecek Exp $ */

/*-
 * Copyright (c) 1995, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.42.14.1 2017/12/03 11:36:54 jdolecek Exp $");

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
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <miscfs/specfs/specdev.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>

#include <sys/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/regnum.h>
#include <machine/vmparam.h>
#include <machine/locore.h>

#include <mips/cache.h>

union linux_ksigframe {
	struct linux_sigframe sf;
#if !defined(__mips_o32)
	struct linux_sigframe32 sf32;
#endif
};

/*
 * To see whether wscons is configured (for virtual console ioctl calls).
 */
#if defined(_KERNEL_OPT)
#include "wsdisplay.h"
#endif
#if (NWSDISPLAY > 0)
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#endif

/*
 * Set set up registers on exec.
 */
void
linux_setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	setregs(l, pack, stack);
	return;
}

#if !defined(__mips_o32)
static void
linux_setup_sigcontext32(struct linux_sigcontext32 *sc,
     const struct trapframe *tf)
{
	for (u_int i = 0; i < 32; i++) {
		sc->lsc_regs[i] = tf->tf_regs[i];
	}
	sc->lsc_mdhi = tf->tf_regs[_R_MULHI];
	sc->lsc_mdlo = tf->tf_regs[_R_MULLO];
	sc->lsc_pc = tf->tf_regs[_R_PC];
}
#endif

static void
linux_setup_sigcontext(struct linux_sigcontext *sc,
     const struct trapframe *tf)
{
	for (u_int i = 0; i < 32; i++) {
		sc->lsc_regs[i] = tf->tf_regs[i];
	}
	sc->lsc_mdhi = tf->tf_regs[_R_MULHI];
	sc->lsc_mdlo = tf->tf_regs[_R_MULLO];
	sc->lsc_pc = tf->tf_regs[_R_PC];
}

/*
 * Send an interrupt to process.
 *
 * Adapted from sys/arch/mips/mips/mips_machdep.c
 *
 * XXX Does not work well yet with RT signals
 *
 */

void
linux_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	const int sig = ksi->ksi_signo;
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
#ifdef __mips_o32
	const int abi = _MIPS_BSD_API_O32;
#else
	const int abi = p->p_md.md_abi;
#endif
	union linux_ksigframe ksf, *sf;
	bool onstack;
	int error;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

#ifdef DEBUG_LINUX
	printf("linux_sendsig()\n");
#endif /* DEBUG_LINUX */

	/*
	 * Do we need to jump onto the signal stack?
	 */
	onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * Signal stack is broken (see at the end of linux_sigreturn), so we do
	 * not use it yet. XXX fix this.
	 */
	onstack = false;

	/*
	 * Allocate space for the signal handler context.
	 */
	sf = (void *)(onstack
	    ? (uintptr_t) l->l_sigstk.ss_sp + l->l_sigstk.ss_size
	    : (uintptr_t) tf->tf_regs[_R_SP]);

	/*
	 * Build stack frame for signal trampoline.
	 */
	memset(&ksf, 0, sizeof ksf);

	/*
	 * This is the signal trampoline used by Linux, we don't use it,
	 * but we set it up in case an application expects it to be there
	 */
	ksf.sf.lsf_code[0] = 0x24020000; /* li	v0, __NR_sigreturn	*/
	ksf.sf.lsf_code[1] = 0x0000000c; /* syscall			*/

	switch (abi) {
	default:
		native_to_linux_sigset(&ksf.sf.lsf_mask, mask);
		linux_setup_sigcontext(&ksf.sf.lsf_sc, tf);
		break;
#if !defined(__mips_o32)
	case _MIPS_BSD_API_O32:
		native_to_linux_sigset(&ksf.sf32.lsf_mask, mask);
		linux_setup_sigcontext32(&ksf.sf32.lsf_sc, tf);
		break;
#endif
	}
	sendsig_reset(l, sig);

	/*
	 * Save signal stack.  XXX broken
	 */
	/* kregs.sc_onstack = l->l_sigstk.ss_flags & SS_ONSTACK; */

	/*
	 * Install the sigframe onto the stack
	 */
	sf -= sizeof(*sf);
	mutex_exit(p->p_lock);
	error = copyout(&ksf, sf, sizeof(ksf));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG_LINUX
		printf("linux_sendsig: stack trashed\n");
#endif /* DEBUG_LINUX */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* Set up the registers to return to sigcode. */
	tf->tf_regs[_R_A0] = native_to_linux_signo[sig];
	tf->tf_regs[_R_A1] = 0;
	tf->tf_regs[_R_A2] = (intptr_t)&sf->sf.lsf_sc;

#ifdef DEBUG_LINUX
	printf("sigcontext is at %p\n", &sf->sf.lsf_sc);
#endif /* DEBUG_LINUX */

	tf->tf_regs[_R_SP] = (intptr_t)sf;
	/* Signal trampoline code is at base of user stack. */
	tf->tf_regs[_R_RA] = (intptr_t)p->p_sigctx.ps_sigcode;
	tf->tf_regs[_R_T9] = (intptr_t)catcher;
	tf->tf_regs[_R_PC] = (intptr_t)catcher;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;

	return;
}

static void
linux_putaway_sigcontext(struct trapframe *tf,
    const struct linux_sigcontext *sc)
{
	for (u_int i = 0; i < 32; i++) {
		tf->tf_regs[i] = sc->lsc_regs[i];
	}
	tf->tf_regs[_R_MULLO] = sc->lsc_mdlo;
	tf->tf_regs[_R_MULHI] = sc->lsc_mdhi;
	tf->tf_regs[_R_PC] = sc->lsc_pc;
}

#ifndef __mips_o32
static void
linux_putaway_sigcontext32(struct trapframe *tf,
    const struct linux_sigcontext32 *sc)
{
	for (u_int i = 0; i < 32; i++) {
		tf->tf_regs[i] = sc->lsc_regs[i];
	}
	tf->tf_regs[_R_MULLO] = sc->lsc_mdlo;
	tf->tf_regs[_R_MULHI] = sc->lsc_mdhi;
	tf->tf_regs[_R_PC] = sc->lsc_pc;
}
#endif

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 */
int
linux_sys_sigreturn(struct lwp *l, const struct linux_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_sigframe *) sf;
	} */
	struct proc *p = l->l_proc;
	union linux_ksigframe ksf, *sf;
#ifdef __mips_o32
	const int abi = _MIPS_BSD_API_O32;
#else
	const int abi = p->p_md.md_abi;
#endif
	linux_sigset_t *lmask;
	sigset_t mask;
	int error;

#ifdef DEBUG_LINUX
	printf("linux_sys_sigreturn()\n");
#endif /* DEBUG_LINUX */

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	sf = (void *)SCARG(uap, sf);

	if ((error = copyin(sf, &ksf, sizeof(ksf))) != 0)
		return (error);

	/* Restore the register context. */
	switch (abi) {
	default:
		lmask = &ksf.sf.lsf_mask;
		linux_putaway_sigcontext(l->l_md.md_utf, &ksf.sf.lsf_sc);
		break;
#if !defined(__mips_o32)
	case _MIPS_BSD_API_O32:
		lmask = &ksf.sf32.lsf_mask;
		linux_putaway_sigcontext32(l->l_md.md_utf, &ksf.sf32.lsf_sc);
		break;
#endif
	}

	mutex_enter(p->p_lock);

	/* Restore signal stack. */
	l->l_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux_to_native_sigset(&mask, lmask);
	(void)sigprocmask1(l, SIG_SETMASK, &mask, 0);

	mutex_exit(p->p_lock);

	return (EJUSTRETURN);
}


int
linux_sys_rt_sigreturn(struct lwp *l, const struct linux_sys_rt_sigreturn_args *v, register_t *retval)
{
	return (ENOSYS);
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
	return 0;
}

/*
 * See above. If a root process tries to set access to an I/O port,
 * just let it have the whole range.
 */
int
linux_sys_ioperm(struct lwp *l, const struct linux_sys_ioperm_args *uap, register_t *retval)
{
	/*
	 * This syscall is not implemented in Linux/Mips: we should not be here
	 */
#ifdef DEBUG_LINUX
	printf("linux_sys_ioperm: should not be here.\n");
#endif /* DEBUG_LINUX */
	return 0;
}

/*
 * wrapper linux_sys_new_uname() -> linux_sys_uname()
 */
int
linux_sys_new_uname(struct lwp *l, const struct linux_sys_new_uname_args *uap, register_t *retval)
{
/*
 * Use this if you want to try Linux emulation with a glibc-2.2
 * or higher. Note that signals will not work
 */
#if 0
        struct linux_sys_uname_args /* {
                syscallarg(struct linux_utsname *) up;
        } */ *uap = v;
        struct linux_utsname luts;

        strlcpy(luts.l_sysname, linux_sysname, sizeof(luts.l_sysname));
        strlcpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
        strlcpy(luts.l_release, "2.4.0", sizeof(luts.l_release));
        strlcpy(luts.l_version, linux_version, sizeof(luts.l_version));
        strlcpy(luts.l_machine, machine, sizeof(luts.l_machine));
        strlcpy(luts.l_domainname, domainname, sizeof(luts.l_domainname));

        return copyout(&luts, SCARG(uap, up), sizeof(luts));
#else
	return linux_sys_uname(l, (const void *)uap, retval);
#endif
}

/*
 * In Linux, cacheflush is currently implemented
 * as a whole cache flush (arguments are ignored)
 * we emulate this broken beahior.
 */
int
linux_sys_cacheflush(struct lwp *l, const struct linux_sys_cacheflush_args *uap, register_t *retval)
{
	mips_icache_sync_all();
	mips_dcache_wbinv_all();
	return 0;
}

/*
 * This system call is depecated in Linux, but
 * some binaries and some libraries use it.
 */
int
linux_sys_sysmips(struct lwp *l, const struct linux_sys_sysmips_args *uap, register_t *retval)
{
	/* {
		syscallarg(long) cmd;
		syscallarg(long) arg1;
		syscallarg(long) arg2;
		syscallarg(long) arg3;
	} */
	int error;

	switch (SCARG(uap, cmd)) {
	case LINUX_SETNAME: {
		char nodename [LINUX___NEW_UTS_LEN + 1];
		int name[2];
		size_t len;

		if ((error = copyinstr((char *)SCARG(uap, arg1), nodename,
		    LINUX___NEW_UTS_LEN, &len)) != 0)
			return error;

		name[0] = CTL_KERN;
		name[1] = KERN_HOSTNAME;
		return (old_sysctl(&name[0], 2, 0, 0, nodename, len, NULL));

		break;
	}
	case LINUX_MIPS_ATOMIC_SET: {
		void *addr;
		int s;
		u_int8_t value = 0;

		addr = (void *)SCARG(uap, arg1);

		s = splhigh();
		/*
		 * No error testing here. This is bad, but Linux does
		 * it like this. The source aknowledge "This is broken"
		 * in a comment...
		 */
		(void) copyin(addr, &value, 1);
		*retval = value;
		value = (u_int8_t) SCARG(uap, arg2);
		error = copyout(&value, addr, 1);
		splx(s);

		return 0;
		break;
	}
	case LINUX_MIPS_FIXADE:		/* XXX not implemented */
		break;
	case LINUX_FLUSH_CACHE:
		mips_icache_sync_all();
		mips_dcache_wbinv_all();
		break;
	case LINUX_MIPS_RDNVRAM:
		return EIO;
		break;
	default:
		return EINVAL;
		break;
	}
#ifdef DEBUG_LINUX
	printf("linux_sys_sysmips(): unimplemented command %d\n",
	    SCARG(uap,cmd));
#endif /* DEBUG_LINUX */
	return 0;
}

int
linux_usertrap(struct lwp *l, vaddr_t trapaddr, void *arg)
{
	return 0;
}

int
linux_sys_set_thread_area(struct lwp *l, const struct linux_sys_set_thread_area_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) tls;
	} */

	return lwp_setprivate(l, SCARG(uap, tls));
}
