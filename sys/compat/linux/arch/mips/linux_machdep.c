/*	$NetBSD: linux_machdep.c,v 1.1 2001/09/22 21:19:10 manu Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
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
#include <miscfs/specfs/specdev.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_hdio.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/regnum.h>
#include <machine/vmparam.h>
#include <machine/locore.h>

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
 * XXX not used at the moment since in sys/kern/exec_conf, LINUX_COMPAT
 * entry uses NetBSD's native setregs instead of linux_setregs
 */
void
linux_setregs(p, pack, stack) 
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{	
	setregs(p, pack, stack);
	return;
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
linux_sendsig(catcher, sig, mask, code)  /* XXX Check me */
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct linux_sigframe *fp;
	struct frame *f;
	int i,onstack;
	struct linux_sigframe sf;

	printf("linux_sendsig()\n");
	f = (struct frame *)p->p_md.md_regs;
	printf("f = %p\n", f);

	/* 
	 * Do we need to jump onto the signal stack? 
	 */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/*
	 * Signal stack is broken (see at the end of linux_sigreturn), so we do
	 * not use it yet. XXX fix this.
	 */
	onstack=0;

	/* 
	 * Allocate space for the signal handler context. 
	 */
	if (onstack)
		fp = (struct linux_sigframe *)
		    ((caddr_t)p->p_sigctx.ps_sigstk.ss_sp
		    + p->p_sigctx.ps_sigstk.ss_size);
	else
		/* cast for _MIPS_BSD_API == _MIPS_BSD_API_LP32_64CLEAN case */
		fp = (struct linux_sigframe *)(u_int32_t)f->f_regs[SP];

	printf("fp = %p, sf = %p\n", fp, &sf);
	/* Build stack frame for signal trampoline. */
	memset(&sf, 0, sizeof sf);
	sf.lsf_ass[0] = 0;			/* XXX */
	sf.lsf_ass[1] = 0;
	sf.lsf_code[0] = 0x24020000;	/* li	v0, __NR_sigreturn	*/
	sf.lsf_code[0] = 0x0000000c;	/* syscall			*/
	native_to_linux_sigset(mask, &sf.lsf_mask);
	for (i=0; i<32; i++)
		sf.lsf_sc.lsc_regs[i] = f->f_regs[i];
	sf.lsf_sc.lsc_mdhi = f->f_regs[MULHI];
	sf.lsf_sc.lsc_mdlo = f->f_regs[MULLO];
	sf.lsf_sc.lsc_pc = f->f_regs[PC];
	sf.lsf_sc.lsc_status = 0;		/* XXX */
	sf.lsf_sc.lsc_ownedfp = 0;		/* XXX */
	sf.lsf_sc.lsc_fpc_csr = f->f_regs[SR];
	sf.lsf_sc.lsc_fpc_eir = f->f_regs[RA];	/* XXX */
	sf.lsf_sc.lsc_cause = f->f_regs[CAUSE];
	sf.lsf_sc.lsc_badvaddr = f->f_regs[BADVADDR];	/* XXX */


	/* 
	 * Save signal stack.  XXX broken
	 */
	/* kregs.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK; */

	/*
	 * Install the sigframe onto the stack
	 */
	fp -= sizeof(struct linux_sigframe);
	if (copyout(&sf, fp, sizeof(sf)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/* Set up the registers to return to sigcode. */
	f->f_regs[A0] = native_to_linux_sig[sig];
	f->f_regs[A1] = 0;
	f->f_regs[A2] = (int)&fp->lsf_sc;

	f->f_regs[SP] = (int)fp;
	f->f_regs[RA] = (int)fp->lsf_code;
	f->f_regs[T9] = (int)catcher;
	f->f_regs[PC] = (int)catcher;

	/* Signal trampoline code is at base of user stack. */

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

	return;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 */
int
linux_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigreturn_args /* {
		syscallarg(struct linux_pt_regs) regs;
	} */ *uap = v;
	struct linux_pt_regs regs, kregs;
	struct linux_sigframe *sf, ksf;
	struct frame *f;
	sigset_t mask;
	int i, error;

	printf("linux_sys_sigreturn()\n");
	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	regs = SCARG(uap, regs);

	kregs = regs;
	/* if ((error = copyin(regs, &kregs, sizeof(kregs))) != 0)
		return (error); */

	sf = (struct linux_sigframe *)kregs.lregs[29];
	if ((error = copyin(sf, &ksf, sizeof(ksf))) != 0)
		return (error);

	/* Restore the register context. */
	f = (struct frame *)p->p_md.md_regs;
	printf("sf = %p, f = %p\n", sf, f);
	for (i=0; i<32; i++)
		f->f_regs[i] = kregs.lregs[i];
	f->f_regs[MULLO] = kregs.llo;
	f->f_regs[MULHI] = kregs.lhi;
	f->f_regs[PC] = kregs.lcp0_spc;
	f->f_regs[BADVADDR] = kregs.lcp0_badvaddr;
	f->f_regs[CAUSE] = kregs.lcp0_cause;

	/* Restore signal stack. */
	p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux_to_native_sigset((linux_sigset_t *)&ksf.lsf_mask, &mask);
	(void)sigprocmask1(p, SIG_SETMASK, &mask, 0);

	return (EJUSTRETURN);
}


int
linux_sys_rt_sigreturn(p, v, retval)  
	struct proc *p;
	void *v;
	register_t *retval;
{
	return 0;
}


#if 0
int
linux_sys_modify_ldt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* 
	 * This syscall is not implemented in Linux/Mips: we should not
	 * be here
	 */
#ifdef DEBUG_LINUX
	printf("linux_sys_modify_ldt: should not be here.\n");	 
#endif
  return 0;
}
#endif

/* 
 * major device numbers remapping
 */
dev_t
linux_fakedev(dev)
	dev_t dev;
{
  /* XXX write me */
  return dev;
}

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call
 */
int
linux_machdepioctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{ 
	return 0;
}

/*
 * See above. If a root process tries to set access to an I/O port,
 * just let it have the whole range.
 */
int
linux_sys_ioperm(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* 
	 * This syscall is not implemented in Linux/Mips: we should not be here
	 */
#ifdef DEBUG_LINUX
	printf("linux_sys_ioperm: should not be here.\n");	 
#endif
	return 0;
}

/*
 * wrapper linux_sys_new_uname() -> linux_sys_uname() 
 */
int	
linux_sys_new_uname(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	return linux_sys_uname(p, v, retval); 
}

/*
 * In Linux, cacheflush is icurrently mplemented
 * as a whole cache flush (arguments are ignored)
 * we emulate this broken beahior.
 */
int
linux_sys_cacheflush(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	MachFlushCache();
	return 0;
}

