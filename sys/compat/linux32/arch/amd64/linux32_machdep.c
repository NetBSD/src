/*	$NetBSD: linux32_machdep.c,v 1.16 2007/12/20 23:02:57 dsl Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: linux32_machdep.c,v 1.16 2007/12/20 23:02:57 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
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
#include <sys/wait.h>
#include <miscfs/specfs/specdev.h>

#include <machine/netbsd32_machdep.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_errno.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_errno.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_exec.h>
#include <compat/linux32/linux32_syscallargs.h>

#include <sys/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/vmparam.h>

extern char linux32_sigcode[1];
extern char linux32_rt_sigcode[1];
extern char linux32_esigcode[1];

extern void (osyscall_return)(void);

static void linux32_save_ucontext(struct lwp *, struct trapframe *,
    const sigset_t *, struct sigaltstack *, struct linux32_ucontext *);
static void linux32_save_sigcontext(struct lwp *, struct trapframe *,
    const sigset_t *, struct linux32_sigcontext *);
static void linux32_rt_sendsig(const ksiginfo_t *, const sigset_t *);
static void linux32_old_sendsig(const ksiginfo_t *, const sigset_t *);
static int linux32_restore_sigcontext(struct lwp *, 
    struct linux32_sigcontext *, register_t *);

void
linux32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	if (SIGACTION(curproc, ksi->ksi_signo).sa_flags & SA_SIGINFO)
		linux32_rt_sendsig(ksi, mask);
	else
		linux32_old_sendsig(ksi, mask);
	return;
}

void
linux32_old_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct linux32_sigframe *fp, frame;
	int onstack, error;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigaltstack *sas = &l->l_sigstk;

	tf = l->l_md.md_regs;
	/* Do we need to jump onto the signal stack? */
	onstack = (sas->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;


	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct linux32_sigframe *)((char *)sas->ss_sp +
		    sas->ss_size);
	else
		fp = (struct linux32_sigframe *)tf->tf_rsp;
	fp--;

	/* Build stack frame for signal trampoline. */
	NETBSD32PTR32(frame.sf_handler, catcher);
	frame.sf_sig = native_to_linux32_signo[sig];

	linux32_save_sigcontext(l, tf, mask, &frame.sf_sc);

	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_gs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_fs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_es = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_ds = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_rip = ((long)p->p_sigctx.ps_sigcode) & 0xffffffff;
	tf->tf_cs = GSEL(GUCODE32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_rflags &= ~(PSL_T|PSL_VM|PSL_AC) & 0xffffffff;
	tf->tf_rsp = (long)fp & 0xffffffff;
	tf->tf_ss = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		sas->ss_flags |= SS_ONSTACK;

	return;
}

void
linux32_rt_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct linux32_rt_sigframe *fp, frame;
	int onstack, error;
	linux32_siginfo_t *lsi;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigaltstack *sas = &l->l_sigstk;

	tf = l->l_md.md_regs;
	/* Do we need to jump onto the signal stack? */
	onstack = (sas->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;


	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct linux32_rt_sigframe *)((char *)sas->ss_sp +
		    sas->ss_size);
	else
		fp = (struct linux32_rt_sigframe *)tf->tf_rsp;
	fp--;

	/* Build stack frame for signal trampoline. */
	NETBSD32PTR32(frame.sf_handler, catcher);
	frame.sf_sig = native_to_linux32_signo[sig];
	NETBSD32PTR32(frame.sf_sip, &fp->sf_si);
	NETBSD32PTR32(frame.sf_ucp, &fp->sf_uc);

	lsi = &frame.sf_si;
	(void)memset(lsi, 0, sizeof(frame.sf_si));
	lsi->lsi_errno = native_to_linux32_errno[ksi->ksi_errno];
	lsi->lsi_code = ksi->ksi_code;
	lsi->lsi_signo = native_to_linux32_signo[frame.sf_sig];
	switch (lsi->lsi_signo) {
	case LINUX32_SIGILL:
	case LINUX32_SIGFPE:
	case LINUX32_SIGSEGV:
	case LINUX32_SIGBUS:
	case LINUX32_SIGTRAP:
		NETBSD32PTR32(lsi->lsi_addr, ksi->ksi_addr);
		break;
	case LINUX32_SIGCHLD:
		lsi->lsi_uid = ksi->ksi_uid;
		lsi->lsi_pid = ksi->ksi_pid;
		lsi->lsi_utime = ksi->ksi_utime;
		lsi->lsi_stime = ksi->ksi_stime;

		/* We use the same codes */
		lsi->lsi_code = ksi->ksi_code;
		/* XXX is that right? */
		lsi->lsi_status = WEXITSTATUS(ksi->ksi_status);
		break;
	case LINUX32_SIGIO:
		lsi->lsi_band = ksi->ksi_band;
		lsi->lsi_fd = ksi->ksi_fd;
		break;
	default:
		lsi->lsi_uid = ksi->ksi_uid;
		lsi->lsi_pid = ksi->ksi_pid;
		if (lsi->lsi_signo == LINUX32_SIGALRM ||
		    lsi->lsi_signo >= LINUX32_SIGRTMIN)
			NETBSD32PTR32(lsi->lsi_value.sival_ptr,
			     ksi->ksi_value.sival_ptr);
		break;
	}

	/* Save register context. */
	linux32_save_ucontext(l, tf, mask, sas, &frame.sf_uc);
	sendsig_reset(l, sig);
	mutex_exit(&p->p_smutex);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_gs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_fs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_es = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_ds = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_rip = (((long)p->p_sigctx.ps_sigcode) +
	    (linux32_rt_sigcode - linux32_sigcode)) & 0xffffffff;
	tf->tf_cs = GSEL(GUCODE32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_rflags &= ~(PSL_T|PSL_VM|PSL_AC) & 0xffffffff;
	tf->tf_rsp = (long)fp & 0xffffffff;
	tf->tf_ss = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		sas->ss_flags |= SS_ONSTACK;

	return;
}

void
linux32_setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;
	struct proc *p = l->l_proc;
	void **retaddr;

	/* If we were using the FPU, forget about it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_lwp(l, 0);

#if defined(USER_LDT) && 0
	pmap_ldt_cleanup(p);
#endif

	netbsd32_adjust_limits(p);

	l->l_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;
	pcb->pcb_savefpu.fp_fxsave.fx_fcw = __Linux_NPXCW__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr = __INITIAL_MXCSR__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr_mask = __INITIAL_MXCSR_MASK__;
	pcb->pcb_fs = 0;
	pcb->pcb_gs = 0;


	p->p_flag |= PK_32;

	tf = l->l_md.md_regs;
	tf->tf_rax = 0;
	tf->tf_rbx = (u_int64_t)p->p_psstr & 0xffffffff;
	tf->tf_rcx = pack->ep_entry & 0xffffffff;
	tf->tf_rdx = 0;
	tf->tf_rsi = 0;
	tf->tf_rdi = 0;
	tf->tf_rbp = 0;
	tf->tf_rsp = stack & 0xffffffff;
	tf->tf_r8 = 0;
	tf->tf_r9 = 0;
	tf->tf_r10 = 0;
	tf->tf_r11 = 0;
	tf->tf_r12 = 0;
	tf->tf_r13 = 0;
	tf->tf_r14 = 0;
	tf->tf_r15 = 0;
	tf->tf_rip = pack->ep_entry & 0xffffffff;
	tf->tf_rflags = PSL_USERSET;
	tf->tf_cs = GSEL(GUCODE32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_ss = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_ds = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_es = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_fs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;
	tf->tf_gs = GSEL(GUDATA32_SEL, SEL_UPL) & 0xffffffff;

	/* XXX frob return address to return via old iret method, not sysret */
	retaddr = (void **)tf - 1;
	*retaddr = (void *)osyscall_return;
	return;
}

static void
linux32_save_ucontext(struct lwp *l, struct trapframe *tf, const sigset_t *mask, struct sigaltstack *sas, struct linux32_ucontext *uc)
{
	uc->uc_flags = 0;
	NETBSD32PTR32(uc->uc_link, NULL);
	native_to_linux32_sigaltstack(&uc->uc_stack, sas);
	linux32_save_sigcontext(l, tf, mask, &uc->uc_mcontext);
	native_to_linux32_sigset(&uc->uc_sigmask, mask);
	(void)memset(&uc->uc_fpregs_mem, 0, sizeof(uc->uc_fpregs_mem));
}

static void
linux32_save_sigcontext(l, tf, mask, sc)
	struct lwp *l; 
	struct trapframe *tf; 
	const sigset_t *mask;
	struct linux32_sigcontext *sc; 
{
	/* Save register context. */
	sc->sc_gs = tf->tf_gs;
	sc->sc_fs = tf->tf_fs;
	sc->sc_es = tf->tf_es;
	sc->sc_ds = tf->tf_ds;
	sc->sc_eflags = tf->tf_rflags;
	sc->sc_edi = tf->tf_rdi;
	sc->sc_esi = tf->tf_rsi;
	sc->sc_esp = tf->tf_rsp;
	sc->sc_ebp = tf->tf_rbp;
	sc->sc_ebx = tf->tf_rbx;
	sc->sc_edx = tf->tf_rdx;
	sc->sc_ecx = tf->tf_rcx;
	sc->sc_eax = tf->tf_rax;
	sc->sc_eip = tf->tf_rip;
	sc->sc_cs = tf->tf_cs;
	sc->sc_esp_at_signal = tf->tf_rsp;
	sc->sc_ss = tf->tf_ss;
	sc->sc_err = tf->tf_err;
	sc->sc_trapno = tf->tf_trapno;
	sc->sc_cr2 = l->l_addr->u_pcb.pcb_cr2;
	NETBSD32PTR32(sc->sc_387, NULL);

	/* Save signal stack. */
	/* Linux doesn't save the onstack flag in sigframe */

	/* Save signal mask. */
	native_to_linux32_old_sigset(&sc->sc_mask, mask);
}

int
linux32_sys_sigreturn(struct lwp *l, const struct linux32_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_sigcontextp_t) scp;
	} */
	struct linux32_sigcontext ctx;
	int error;

	if ((error = copyin(SCARG_P32(uap, scp), &ctx, sizeof(ctx))) != 0)
		return error;

	return linux32_restore_sigcontext(l, &ctx, retval);
}

int
linux32_sys_rt_sigreturn(struct lwp *l, const struct linux32_sys_rt_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_ucontextp_t) ucp;
	} */
	struct linux32_ucontext ctx;
	int error;

	if ((error = copyin(SCARG_P32(uap, ucp), &ctx, sizeof(ctx))) != 0)
		return error;

	return linux32_restore_sigcontext(l, &ctx.uc_mcontext, retval);
}

static int
linux32_restore_sigcontext(l, scp, retval)
	struct lwp *l;
	struct linux32_sigcontext *scp;
	register_t *retval;
{	
	struct trapframe *tf;
	struct proc *p = l->l_proc;
	struct sigaltstack *sas = &l->l_sigstk;
	sigset_t mask;
	ssize_t ss_gap;

	/* Restore register context. */
	tf = l->l_md.md_regs;

	/*
	 * Check for security violations.  If we're returning to
	 * protected mode, the CPU will validate the segment registers
	 * automatically and generate a trap on violations.  We handle
	 * the trap, rather than doing all of the checking here.
	 */
	if (((scp->sc_eflags ^ tf->tf_rflags) & PSL_USERSTATIC) != 0 ||
	    !USERMODE(scp->sc_cs, scp->sc_eflags))
		return EINVAL;

	if (scp->sc_fs != 0 && !VALID_USER_DSEL32(scp->sc_fs))
		return EINVAL;

	if (scp->sc_gs != 0 && !VALID_USER_DSEL32(scp->sc_gs))
		return EINVAL;

	if (scp->sc_es != 0 && !VALID_USER_DSEL32(scp->sc_es))
		return EINVAL;

	if (!VALID_USER_DSEL32(scp->sc_ds) || 
	    !VALID_USER_DSEL32(scp->sc_ss))
		return EINVAL;

	if (scp->sc_eip >= VM_MAXUSER_ADDRESS32)
		return EINVAL;

	tf->tf_gs = (register_t)scp->sc_gs & 0xffffffff;
	tf->tf_fs = (register_t)scp->sc_fs & 0xffffffff;
	tf->tf_es = (register_t)scp->sc_es & 0xffffffff;
	tf->tf_ds = (register_t)scp->sc_ds & 0xffffffff;
	tf->tf_rflags &= ~PSL_USER;
	tf->tf_rflags |= ((register_t)scp->sc_eflags & PSL_USER);
	tf->tf_rdi = (register_t)scp->sc_edi & 0xffffffff;
	tf->tf_rsi = (register_t)scp->sc_esi & 0xffffffff;
	tf->tf_rbp = (register_t)scp->sc_ebp & 0xffffffff;
	tf->tf_rbx = (register_t)scp->sc_ebx & 0xffffffff;
	tf->tf_rdx = (register_t)scp->sc_edx & 0xffffffff;
	tf->tf_rcx = (register_t)scp->sc_ecx & 0xffffffff;
	tf->tf_rax = (register_t)scp->sc_eax & 0xffffffff;
	tf->tf_rip = (register_t)scp->sc_eip & 0xffffffff;
	tf->tf_cs = (register_t)scp->sc_cs & 0xffffffff;
	tf->tf_rsp = (register_t)scp->sc_esp_at_signal & 0xffffffff;
	tf->tf_ss = (register_t)scp->sc_ss & 0xffffffff;

	mutex_enter(&p->p_smutex);

	/* Restore signal stack. */
	ss_gap = (ssize_t)
	    ((char *)NETBSD32IPTR64(scp->sc_esp_at_signal) 
	     - (char *)sas->ss_sp);
	if (ss_gap >= 0 && ss_gap < sas->ss_size)
		sas->ss_flags |= SS_ONSTACK;
	else
		sas->ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux32_old_to_native_sigset(&mask, &scp->sc_mask);
	(void) sigprocmask1(l, SIG_SETMASK, &mask, 0);

	mutex_exit(&p->p_smutex);

#ifdef DEBUG_LINUX
	printf("linux32_sigreturn: rip = 0x%lx, rsp = 0x%lx, flags = 0x%lx\n",
	    tf->tf_rip, tf->tf_rsp, tf->tf_rflags);
#endif
	return EJUSTRETURN;
}
