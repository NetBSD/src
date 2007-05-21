/*	$NetBSD: linux_machdep.c,v 1.22 2007/05/21 15:35:47 christos Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.22 2007/05/21 15:35:47 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/ptrace.h> /* for process_read_fpregs() */
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ucontext.h>
#include <sys/conf.h>

#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/fpu.h>
#include <machine/mcontext.h>
#include <machine/specialreg.h>
#include <machine/vmparam.h>

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


#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_prctl.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>

static void linux_buildcontext(struct lwp *, void *, void *);

void
linux_setregs(l, epp, stack) 
        struct lwp *l;
	struct exec_package *epp;
	u_long stack; 
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;

	/* If we were using the FPU, forget about it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_lwp(l, 0);

	l->l_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;
	pcb->pcb_savefpu.fp_fxsave.fx_fcw = __NetBSD_NPXCW__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr = __INITIAL_MXCSR__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr_mask = __INITIAL_MXCSR_MASK__;
	pcb->pcb_fs = 0;
	pcb->pcb_gs = 0;

	l->l_proc->p_flag &= ~PK_32;

	tf = l->l_md.md_regs;
	tf->tf_rax = 0;
	tf->tf_rbx = 0;
	tf->tf_rcx = epp->ep_entry;
	tf->tf_rdx = 0;
	tf->tf_rsi = 0;
	tf->tf_rdi = 0;
	tf->tf_rbp = 0;
	tf->tf_rsp = stack;
	tf->tf_r8 = 0;
	tf->tf_r9 = 0;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_r15 = 0;
	tf->tf_rip = epp->ep_entry;
	tf->tf_rflags = PSL_USERSET;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = 0;
	tf->tf_es = 0;
	tf->tf_fs = 0;
	tf->tf_gs = 0;

	return;
}

void    
linux_sendsig(ksi, mask)
	const ksiginfo_t *ksi;
	const sigset_t *mask;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct linux_rt_sigframe *sfp, sigframe;
	struct linux__fpstate *fpsp, fpstate;
	struct fpreg fpregs;
	struct trapframe *tf = l->l_md.md_regs;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	linux_sigset_t lmask;
	char *sp;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	
	/* Allocate space for the signal handler context. */
	if (onstack)
		sp = ((char *)l->l_sigstk.ss_sp +
		    l->l_sigstk.ss_size);
	else
		sp = (char *)tf->tf_rsp - 128;

	/* 
	 * Save FPU state, if any 
	 */
	if (l->l_md.md_flags & MDP_USEDFPU) {
		sp = (char *)
		    (((long)sp - sizeof(struct linux__fpstate)) & ~0xfUL);
		fpsp = (struct linux__fpstate *)sp;
	} else
		fpsp = NULL;

	/* 
	 * Populate the rt_sigframe 
	 */
	sp = (char *)
	    ((((long)sp - sizeof(struct linux_rt_sigframe)) & ~0xfUL) - 8);
	sfp = (struct linux_rt_sigframe *)sp;

	bzero(&sigframe, sizeof(sigframe));
	if (ps->sa_sigdesc[sig].sd_vers != 0)
		sigframe.pretcode = 
		    (char *)(u_long)ps->sa_sigdesc[sig].sd_tramp;
	else
		sigframe.pretcode = NULL;

	/* 
	 * The user context 
	 */
	sigframe.uc.luc_flags = 0;
	sigframe.uc.luc_link = NULL;

	/* This is used regardless of SA_ONSTACK in Linux */
	sigframe.uc.luc_stack.ss_sp = l->l_sigstk.ss_sp;
	sigframe.uc.luc_stack.ss_size = l->l_sigstk.ss_size;
	sigframe.uc.luc_stack.ss_flags = 0;
	if (l->l_sigstk.ss_flags & SS_ONSTACK)
		sigframe.uc.luc_stack.ss_flags |= LINUX_SS_ONSTACK;
	if (l->l_sigstk.ss_flags & SS_DISABLE)
		sigframe.uc.luc_stack.ss_flags |= LINUX_SS_DISABLE;

	sigframe.uc.luc_mcontext.r8 = tf->tf_r8;
	sigframe.uc.luc_mcontext.r9 = tf->tf_r9;
	sigframe.uc.luc_mcontext.r10 = tf->tf_r10;
	sigframe.uc.luc_mcontext.r11 = tf->tf_r11;
	sigframe.uc.luc_mcontext.r12 = tf->tf_r12;
	sigframe.uc.luc_mcontext.r13 = tf->tf_r13;
	sigframe.uc.luc_mcontext.r14 = tf->tf_r14;
	sigframe.uc.luc_mcontext.r15 = tf->tf_r15;
	sigframe.uc.luc_mcontext.rdi = tf->tf_rdi;
	sigframe.uc.luc_mcontext.rsi = tf->tf_rsi;
	sigframe.uc.luc_mcontext.rbp = tf->tf_rbp;
	sigframe.uc.luc_mcontext.rbx = tf->tf_rbx;
	sigframe.uc.luc_mcontext.rdx = tf->tf_rdx;
	sigframe.uc.luc_mcontext.rcx = tf->tf_rcx;
	sigframe.uc.luc_mcontext.rsp = tf->tf_rsp;
	sigframe.uc.luc_mcontext.rip = tf->tf_rip;
	sigframe.uc.luc_mcontext.eflags = tf->tf_rflags;
	sigframe.uc.luc_mcontext.cs = tf->tf_cs;
	sigframe.uc.luc_mcontext.gs = tf->tf_gs;
	sigframe.uc.luc_mcontext.fs = tf->tf_fs;
	sigframe.uc.luc_mcontext.err = tf->tf_err;
	sigframe.uc.luc_mcontext.trapno = tf->tf_trapno;
	native_to_linux_sigset(&lmask, mask);
	sigframe.uc.luc_mcontext.oldmask = lmask.sig[0];
	sigframe.uc.luc_mcontext.cr2 = (long)l->l_addr->u_pcb.pcb_onfault;
	sigframe.uc.luc_mcontext.fpstate = fpsp;
	native_to_linux_sigset(&sigframe.uc.luc_sigmask, mask);

	/* 
	 * the siginfo structure
	 */
	sigframe.info.lsi_signo = native_to_linux_signo[sig];
	sigframe.info.lsi_errno = native_to_linux_errno[ksi->ksi_errno];
	sigframe.info.lsi_code = ksi->ksi_code;

	/* XXX This is a rought conversion, taken from i386 code */
	switch (sigframe.info.lsi_signo) {
	case LINUX_SIGILL:
	case LINUX_SIGFPE:
	case LINUX_SIGSEGV:
	case LINUX_SIGBUS:
	case LINUX_SIGTRAP:
		sigframe.info._sifields._sigfault._addr = ksi->ksi_addr;
		break;
	case LINUX_SIGCHLD:
		sigframe.info._sifields._sigchld._pid = ksi->ksi_pid;
		sigframe.info._sifields._sigchld._uid = ksi->ksi_uid;
		sigframe.info._sifields._sigchld._utime = ksi->ksi_utime;
		sigframe.info._sifields._sigchld._stime = ksi->ksi_stime;

		if (WCOREDUMP(ksi->ksi_status)) {
			sigframe.info.lsi_code = LINUX_CLD_DUMPED;
			sigframe.info._sifields._sigchld._status =
			    _WSTATUS(ksi->ksi_status);
		} else if (_WSTATUS(ksi->ksi_status)) {
			sigframe.info.lsi_code = LINUX_CLD_KILLED;
			sigframe.info._sifields._sigchld._status =
			    _WSTATUS(ksi->ksi_status);
		} else {
			sigframe.info.lsi_code = LINUX_CLD_EXITED;
			sigframe.info._sifields._sigchld._status =
			    ((ksi->ksi_status & 0xff00U) >> 8);
		}
		break;
	case LINUX_SIGIO:
		sigframe.info._sifields._sigpoll._band = ksi->ksi_band;
		sigframe.info._sifields._sigpoll._fd = ksi->ksi_fd;
		break;
	default:
		sigframe.info._sifields._sigchld._pid = ksi->ksi_pid;
		sigframe.info._sifields._sigchld._uid = ksi->ksi_uid;
		if ((sigframe.info.lsi_signo == LINUX_SIGALRM) ||
		    (sigframe.info.lsi_signo >= LINUX_SIGRTMIN))
			sigframe.info._sifields._timer._sigval.sival_ptr =
			     ksi->ksi_value.sival_ptr;
		break;
	}

	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = 0;

	/* 
	 * Save FPU state, if any 
	 */
	if (fpsp != NULL) {
		(void)process_read_fpregs(l, &fpregs);
		bzero(&fpstate, sizeof(fpstate));
		fpstate.cwd = fpregs.fp_fcw;
		fpstate.swd = fpregs.fp_fsw;
		fpstate.twd = fpregs.fp_ftw;
		fpstate.fop = fpregs.fp_fop;
		fpstate.rip = fpregs.fp_rip;
		fpstate.rdp = fpregs.fp_rdp;
		fpstate.mxcsr = fpregs.fp_mxcsr;
		fpstate.mxcsr_mask = fpregs.fp_mxcsr_mask;
		memcpy(&fpstate.st_space, &fpregs.fp_st, 
		    sizeof(fpstate.st_space));
		memcpy(&fpstate.xmm_space, &fpregs.fp_xmm, 
		    sizeof(fpstate.xmm_space));
		error = copyout(&fpstate, fpsp, sizeof(fpstate));
	}

	if (error == 0)
		error = copyout(&sigframe, sp, sizeof(sigframe));

	mutex_enter(&p->p_smutex);

	if (error != 0) {
		sigexit(l, SIGILL);
		return;
	}	

	linux_buildcontext(l, catcher, sp);
	tf->tf_rdi = sigframe.info.lsi_signo;
	tf->tf_rax = 0;
	tf->tf_rsi = (long)&sfp->info;
	tf->tf_rdx = (long)&sfp->uc;

	/*
	 * Remember we use signal stack
	 */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	return;
}

int     
linux_sys_modify_ldt(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{ 
	printf("linux_sys_modify_ldt\n");
	return 0;
}

int     
linux_sys_iopl(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{  
	return 0;
}

int     
linux_sys_ioperm(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{    
	return 0;
}

dev_t
linux_fakedev(dev, raw)
        dev_t dev;
	int raw;
{

       extern const struct cdevsw ptc_cdevsw, pts_cdevsw;
       const struct cdevsw *cd = cdevsw_lookup(dev);

       if (raw) {
#if (NWSDISPLAY > 0)
	       extern const struct cdevsw wsdisplay_cdevsw;
	       if (cd == &wsdisplay_cdevsw)
		       return makedev(LINUX_CONS_MAJOR, (minor(dev) + 1));
#endif
       }

       if (cd == &ptc_cdevsw)
	       return makedev(LINUX_PTC_MAJOR, minor(dev));
       if (cd == &pts_cdevsw)
	       return makedev(LINUX_PTS_MAJOR, minor(dev));

	return ((minor(dev) & 0xff) | ((major(dev) & 0xfff) << 8)
	    | (((unsigned long long int) (minor(dev) & ~0xff)) << 12)
	    | (((unsigned long long int) (major(dev) & ~0xfff)) << 32));
}

int  
linux_machdepioctl(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{  
	return 0;
}

int
linux_sys_rt_sigreturn(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{  
	struct linux_ucontext *luctx;
	struct trapframe *tf = l->l_md.md_regs;
	struct linux_sigcontext *lsigctx;
	struct linux__fpstate fpstate;
	struct linux_rt_sigframe frame, *fp;
	ucontext_t uctx;
	mcontext_t *mctx;
	struct fxsave64 *fxsave;
	int error;

	fp = (struct linux_rt_sigframe *)(tf->tf_rsp - 8);
	if ((error = copyin(fp, &frame, sizeof(frame))) != 0) {
		mutex_enter(&l->l_proc->p_smutex);
		sigexit(l, SIGILL);
		return error;
	}
	luctx = &frame.uc;
	lsigctx = &luctx->luc_mcontext;

	bzero(&uctx, sizeof(uctx));
	mctx = (mcontext_t *)&uctx.uc_mcontext;
	fxsave = (struct fxsave64 *)&mctx->__fpregs;

	/* 
	 * Set the flags. Linux always have CPU, stack and signal state,
	 * FPU is optional. uc_flags is not used to tell what we have.
	 */
	uctx.uc_flags = (_UC_SIGMASK|_UC_CPU|_UC_STACK|_UC_CLRSTACK);
	if (lsigctx->fpstate != NULL)
		uctx.uc_flags |= _UC_FPU;
	uctx.uc_link = NULL;

	/*
	 * Signal set 
	 */
	linux_to_native_sigset(&uctx.uc_sigmask, &luctx->luc_sigmask);

	/*
	 * CPU state
	 */
	mctx->__gregs[_REG_R8] = lsigctx->r8;
	mctx->__gregs[_REG_R9] = lsigctx->r9;
	mctx->__gregs[_REG_R10] = lsigctx->r10;
	mctx->__gregs[_REG_R11] = lsigctx->r11;
	mctx->__gregs[_REG_R12] = lsigctx->r12;
	mctx->__gregs[_REG_R13] = lsigctx->r13;
	mctx->__gregs[_REG_R14] = lsigctx->r14;
	mctx->__gregs[_REG_R15] = lsigctx->r15;
	mctx->__gregs[_REG_RDI] = lsigctx->rdi;
	mctx->__gregs[_REG_RSI] = lsigctx->rsi;
	mctx->__gregs[_REG_RBP] = lsigctx->rbp;
	mctx->__gregs[_REG_RBX] = lsigctx->rbx;
	mctx->__gregs[_REG_RAX] = tf->tf_rax;
	mctx->__gregs[_REG_RDX] = lsigctx->rdx;
	mctx->__gregs[_REG_RCX] = lsigctx->rcx;
	mctx->__gregs[_REG_RIP] = lsigctx->rip;
	mctx->__gregs[_REG_RFL] = lsigctx->eflags;
	mctx->__gregs[_REG_CS] = lsigctx->cs;
	mctx->__gregs[_REG_GS] = lsigctx->gs;
	mctx->__gregs[_REG_FS] = lsigctx->fs;
	mctx->__gregs[_REG_ERR] = lsigctx->err;
	mctx->__gregs[_REG_TRAPNO] = lsigctx->trapno;
	mctx->__gregs[_REG_ES] = tf->tf_es;
	mctx->__gregs[_REG_DS] = tf->tf_ds;
	mctx->__gregs[_REG_URSP] = lsigctx->rsp; /* XXX */
	mctx->__gregs[_REG_SS] = tf->tf_ss;

	/*
	 * FPU state 
	 */
	if (lsigctx->fpstate != NULL) {
		error = copyin(lsigctx->fpstate, &fpstate, sizeof(fpstate));
		if (error != 0) {
			mutex_enter(&l->l_proc->p_smutex);
			sigexit(l, SIGILL);
			return error;
		}

		fxsave->fx_fcw = fpstate.cwd;
		fxsave->fx_fsw = fpstate.swd;
		fxsave->fx_ftw = fpstate.twd;
		fxsave->fx_fop = fpstate.fop;
		fxsave->fx_rip = fpstate.rip;
		fxsave->fx_rdp = fpstate.rdp;
		fxsave->fx_mxcsr = fpstate.mxcsr;
		fxsave->fx_mxcsr_mask = fpstate.mxcsr_mask;
		memcpy(&fxsave->fx_st, &fpstate.st_space, 
		    sizeof(fxsave->fx_st));
		memcpy(&fxsave->fx_xmm, &fpstate.xmm_space, 
		    sizeof(fxsave->fx_xmm));
	}

	/*
	 * And the stack
	 */
	uctx.uc_stack.ss_flags = 0;
	if (luctx->luc_stack.ss_flags & LINUX_SS_ONSTACK);
		uctx.uc_stack.ss_flags = SS_ONSTACK;

	if (luctx->luc_stack.ss_flags & LINUX_SS_DISABLE);
		uctx.uc_stack.ss_flags = SS_DISABLE;

	uctx.uc_stack.ss_sp = luctx->luc_stack.ss_sp;
	uctx.uc_stack.ss_size = luctx->luc_stack.ss_size;

	/*
	 * And let setucontext deal with that.
	 */
	mutex_enter(&l->l_proc->p_smutex);
	error = setucontext(l, &uctx);
	mutex_exit(&l->l_proc->p_smutex);

	return error;
}

int
linux_sys_arch_prctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_arch_prctl_args /* {
		syscallarg(int) code;
		syscallarg(unsigned long) addr;
	} */ *uap = v;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf = l->l_md.md_regs;
	int error;
	uint64_t taddr;

	switch(SCARG(uap, code)) {
	case LINUX_ARCH_SET_GS:
		taddr = SCARG(uap, addr);
		if (taddr >= VM_MAXUSER_ADDRESS)
			return EINVAL;
		pcb->pcb_gs = taddr;
		pcb->pcb_flags |= PCB_GS64;
		if (l == curlwp)
			wrmsr(MSR_KERNELGSBASE, taddr);
		break;

	case LINUX_ARCH_GET_GS:
		if (pcb->pcb_flags & PCB_GS64)
			taddr = pcb->pcb_gs;
		else {
			error = memseg_baseaddr(l, tf->tf_fs, NULL, 0, &taddr);
			if (error != 0)
				return error;
		}
		error = copyout(&taddr, (char *)SCARG(uap, addr), 8);
		if (error != 0)
			return error;
		break;

	case LINUX_ARCH_SET_FS:
		taddr = SCARG(uap, addr);
		if (taddr >= VM_MAXUSER_ADDRESS)
			return EINVAL;
		pcb->pcb_fs = taddr;
		pcb->pcb_flags |= PCB_FS64;
		if (l == curlwp)
			wrmsr(MSR_FSBASE, taddr);
		break;

	case LINUX_ARCH_GET_FS:
		if (pcb->pcb_flags & PCB_FS64)
			taddr = pcb->pcb_fs;
		else {
			error = memseg_baseaddr(l, tf->tf_fs, NULL, 0, &taddr);
			if (error != 0)
				return error;
		}
		error = copyout(&taddr, (char *)SCARG(uap, addr), 8);
		if (error != 0)
			return error;
		break;

	default:
#ifdef DEBUG_LINUX
		printf("linux_sys_arch_prctl: unexpected code %d\n", 
		    SCARG(uap, code));
#endif
		return EINVAL;
	}

	return 0;
}

const int linux_vsyscall_to_syscall[] = {
	LINUX_SYS_gettimeofday,
	LINUX_SYS_time,
	LINUX_SYS_nosys,
	LINUX_SYS_nosys,
};

int
linux_usertrap(struct lwp *l, vaddr_t trapaddr, void *arg)
{
	struct trapframe *tf = arg;
	uint64_t retaddr;
	int vsyscallnr;

	/*
	 * Check for a vsyscall. %rip must be the fault address,
	 * and the address must be in the Linux vsyscall area.
	 * Also, vsyscalls are only done at 1024-byte boundaries.
	 */

	if (__predict_true(trapaddr < LINUX_VSYSCALL_START))
		return 0;

	if (trapaddr != tf->tf_rip)
		return 0;

	if ((tf->tf_rip & (LINUX_VSYSCALL_SIZE - 1)) != 0)
		return 0;

	vsyscallnr = (tf->tf_rip - LINUX_VSYSCALL_START) / LINUX_VSYSCALL_SIZE;

	if (vsyscallnr > LINUX_VSYSCALL_MAXNR)
		return 0;

	/*
	 * Get the return address from the top of the stack,
	 * and fix up the return address.
	 * This assumes the faulting instruction was callq *reg,
	 * which is the only way that vsyscalls are ever entered.
	 */
	if (copyin((void *)tf->tf_rsp, &retaddr, sizeof retaddr) != 0)
		return 0;
	tf->tf_rip = retaddr;
	tf->tf_rax = linux_vsyscall_to_syscall[vsyscallnr];
	tf->tf_rsp += 8;	/* "pop" the return address */

#if 0
	printf("usertrap: rip %p rsp %p retaddr %p vsys %d sys %d\n",
	    (void *)tf->tf_rip, (void *)tf->tf_rsp, (void *)retaddr,
	    vsyscallnr, (int)tf->tf_rax);
#endif

	(*l->l_proc->p_md.md_syscall)(tf);

	return 1;
}

static void
linux_buildcontext(struct lwp *l, void *catcher, void *f)
{
	struct trapframe *tf = l->l_md.md_regs;

	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_rip = (u_int64_t)catcher;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_rflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_rsp = (u_int64_t)f;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
}

void *
linux_get_newtls(l)
	struct lwp *l;
{
	struct trapframe *tf = l->l_md.md_regs;

	return (void *)tf->tf_r8;
}

int
linux_set_newtls(l, tls)
	struct lwp *l;
	void *tls;
{
	struct linux_sys_arch_prctl_args cup;
	register_t retval;

	SCARG(&cup, code) = LINUX_ARCH_SET_FS;
	SCARG(&cup, addr) = (unsigned long)tls;

	return linux_sys_arch_prctl(l, &cup, &retval);
}
