/*	$NetBSD: linux_machdep.c,v 1.96 2003/09/06 22:09:21 christos Exp $	*/

/*-
 * Copyright (c) 1995, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_machdep.c,v 1.96 2003/09/06 22:09:21 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#include "opt_user_ldt.h"
#endif

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
#include <sys/sa.h>
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
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/vm86.h>
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
#if defined(_KERNEL_OPT)
#include "opt_xserver.h"
#endif
#endif

#ifdef USER_LDT
#include <machine/cpu.h>
int linux_read_ldt __P((struct lwp *, struct linux_sys_modify_ldt_args *,
    register_t *));
int linux_write_ldt __P((struct lwp *, struct linux_sys_modify_ldt_args *,
    register_t *));
#endif

#ifdef DEBUG_LINUX
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif

static struct biosdisk_info *fd2biosinfo __P((struct proc *, struct file *));
extern struct disklist *i386_alldisks;
static void linux_save_ucontext __P((struct lwp *, struct trapframe *,
    sigset_t *, struct sigaltstack *, struct linux_ucontext *));
static void linux_save_sigcontext __P((struct lwp *, struct trapframe *,
    sigset_t *, struct linux_sigcontext *));
static int linux_restore_sigcontext __P((struct lwp *,
    struct linux_sigcontext *, register_t *));
static void linux_rt_sendsig __P((int, sigset_t *, u_long));
static void linux_old_sendsig __P((int, sigset_t *, u_long));

extern char linux_sigcode[], linux_rt_sigcode[];
/*
 * Deal with some i386-specific things in the Linux emulation code.
 */

void
linux_setregs(l, epp, stack)
	struct lwp *l;
	struct exec_package *epp;
	u_long stack;
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;

#if NNPX > 0
	/* If we were using the FPU, forget about it. */
	if (npxproc == l)
		npxdrop();
#endif

#ifdef USER_LDT
	pmap_ldt_cleanup(l);
#endif

	l->l_md.md_flags &= ~MDP_USEDFPU;

	if (i386_use_fxsave) {
		pcb->pcb_savefpu.sv_xmm.sv_env.en_cw = __Linux_NPXCW__;
		pcb->pcb_savefpu.sv_xmm.sv_env.en_mxcsr = __INITIAL_MXCSR__;
	} else
		pcb->pcb_savefpu.sv_87.sv_env.en_cw = __Linux_NPXCW__;

	tf = l->l_md.md_regs;
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_edi = 0;
	tf->tf_esi = 0;
	tf->tf_ebp = 0;
	tf->tf_ebx = (int)l->l_proc->p_psstr;
	tf->tf_edx = 0;
	tf->tf_ecx = 0;
	tf->tf_eax = 0;
	tf->tf_eip = epp->ep_entry;
	tf->tf_cs = GSEL(GUCODEBIG_SEL, SEL_UPL);
	tf->tf_eflags = PSL_USERSET;
	tf->tf_esp = stack;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */

void
linux_sendsig(ksiginfo_t *ksi, sigset_t *mask)
{
	if (SIGACTION(curproc, ksi->ksi_signo).sa_flags & SA_SIGINFO)
		linux_rt_sendsig(ksi->ksi_signo, mask, ksi->ksi_trap);
	else
		linux_old_sendsig(ksi->ksi_signo, mask, ksi->ksi_trap);
}


static void
linux_save_ucontext(l, tf, mask, sas, uc)
	struct lwp *l;
	struct trapframe *tf;
	sigset_t *mask;
	struct sigaltstack *sas;
	struct linux_ucontext *uc;
{
	uc->uc_flags = 0;
	uc->uc_link = NULL;
	native_to_linux_sigaltstack(&uc->uc_stack, sas);
	linux_save_sigcontext(l, tf, mask, &uc->uc_mcontext);
	native_to_linux_sigset(&uc->uc_sigmask, mask);
	(void)memset(&uc->uc_fpregs_mem, 0, sizeof(uc->uc_fpregs_mem));
}

static void
linux_save_sigcontext(l, tf, mask, sc)
	struct lwp *l;
	struct trapframe *tf;
	sigset_t *mask;
	struct linux_sigcontext *sc;
{
	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		sc->sc_gs = tf->tf_vm86_gs;
		sc->sc_fs = tf->tf_vm86_fs;
		sc->sc_es = tf->tf_vm86_es;
		sc->sc_ds = tf->tf_vm86_ds;
		sc->sc_eflags = get_vflags(l);
	} else
#endif
	{
		sc->sc_gs = tf->tf_gs;
		sc->sc_fs = tf->tf_fs;		
		sc->sc_es = tf->tf_es;
		sc->sc_ds = tf->tf_ds;
		sc->sc_eflags = tf->tf_eflags;
	}
	sc->sc_edi = tf->tf_edi;
	sc->sc_esi = tf->tf_esi;
	sc->sc_esp = tf->tf_esp;
	sc->sc_ebp = tf->tf_ebp;
	sc->sc_ebx = tf->tf_ebx;
	sc->sc_edx = tf->tf_edx;
	sc->sc_ecx = tf->tf_ecx;
	sc->sc_eax = tf->tf_eax;
	sc->sc_eip = tf->tf_eip;
	sc->sc_cs = tf->tf_cs;
	sc->sc_esp_at_signal = tf->tf_esp;
	sc->sc_ss = tf->tf_ss;
	sc->sc_err = tf->tf_err;
	sc->sc_trapno = tf->tf_trapno;
	sc->sc_cr2 = l->l_addr->u_pcb.pcb_cr2;
	sc->sc_387 = NULL;

	/* Save signal stack. */
	/* Linux doesn't save the onstack flag in sigframe */

	/* Save signal mask. */
	native_to_linux_old_sigset(&sc->sc_mask, mask);
}

static void
linux_rt_sendsig(sig, mask, code)
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct linux_rt_sigframe *fp, frame;
	int onstack;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigaltstack *sas = &p->p_sigctx.ps_sigstk;

	tf = l->l_md.md_regs;
	/* Do we need to jump onto the signal stack? */
	onstack = (sas->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;


	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct linux_rt_sigframe *)((caddr_t)sas->ss_sp +
		    sas->ss_size);
	else
		fp = (struct linux_rt_sigframe *)tf->tf_esp;
	fp--;

	DPRINTF(("rt: onstack = %d, fp = %p sig = %d eip = 0x%x cr2 = 0x%x\n",
	    onstack, fp, sig, tf->tf_eip, l->l_addr->u_pcb.pcb_cr2));

	/* Build stack frame for signal trampoline. */
	frame.sf_handler = catcher;
	frame.sf_sig = native_to_linux_signo[sig];
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;

	(void)memset(&frame.sf_si, 0, sizeof(frame.sf_si));
	/*
	 * XXX: We'll fake bit of it here, all of the following
	 * info is a bit bogus, because we don't have the
	 * right info passed to us from the trap.
	 */
	switch (frame.sf_si.lsi_signo = frame.sf_sig) {
	case LINUX_SIGSEGV:
		frame.sf_si.lsi_code = LINUX_SEGV_MAPERR;
		break;
	case LINUX_SIGBUS:
		frame.sf_si.lsi_code = LINUX_BUS_ADRERR;
		break;
	case LINUX_SIGTRAP:
		frame.sf_si.lsi_code = LINUX_TRAP_BRKPT;
		break;
	case LINUX_SIGCHLD:
	case LINUX_SIGIO:
	default:
		frame.sf_si.lsi_signo = 0;
		break;
	}

	/* Save register context. */
	linux_save_ucontext(l, tf, mask, sas, &frame.sf_uc);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
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
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = ((int)p->p_sigctx.ps_sigcode) + 
	    (linux_rt_sigcode - linux_sigcode);
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		sas->ss_flags |= SS_ONSTACK;
}

static void
linux_old_sendsig(sig, mask, code)
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct trapframe *tf;
	struct linux_sigframe *fp, frame;
	int onstack;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct sigaltstack *sas = &p->p_sigctx.ps_sigstk;

	tf = l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack = (sas->ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct linux_sigframe *) ((caddr_t)sas->ss_sp +
		    sas->ss_size);
	else
		fp = (struct linux_sigframe *)tf->tf_esp;
	fp--;

	DPRINTF(("old: onstack = %d, fp = %p sig = %d eip = 0x%x cr2 = 0x%x\n",
	    onstack, fp, sig, tf->tf_eip, l->l_addr->u_pcb.pcb_cr2));

	/* Build stack frame for signal trampoline. */
	frame.sf_handler = catcher;
	frame.sf_sig = native_to_linux_signo[sig];

	linux_save_sigcontext(l, tf, mask, &frame.sf_sc);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
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
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)p->p_sigctx.ps_sigcode;
	tf->tf_cs = GSEL(GUCODEBIG_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		sas->ss_flags |= SS_ONSTACK;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
linux_sys_rt_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rt_sigreturn_args /* {
		syscallarg(struct linux_ucontext *) ucp;
	} */ *uap = v;
	struct linux_ucontext context, *ucp = SCARG(uap, ucp);
	int error;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	if ((error = copyin(ucp, &context, sizeof(*ucp))) != 0)
		return error;

	/* XXX XAX we can do better here by using more of the ucontext */
	return linux_restore_sigcontext(l, &context.uc_mcontext, retval);
}

int
linux_sys_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigreturn_args /* {
		syscallarg(struct linux_sigcontext *) scp;
	} */ *uap = v;
	struct linux_sigcontext context, *scp = SCARG(uap, scp);
	int error;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	if ((error = copyin((caddr_t)scp, &context, sizeof(*scp))) != 0)
		return error;
	return linux_restore_sigcontext(l, &context, retval);
}

static int
linux_restore_sigcontext(l, scp, retval)
	struct lwp *l;
	struct linux_sigcontext *scp;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct sigaltstack *sas = &p->p_sigctx.ps_sigstk;
	struct trapframe *tf;
	sigset_t mask;
	ssize_t ss_gap;
	/* Restore register context. */
	tf = l->l_md.md_regs;

	DPRINTF(("sigreturn enter esp=%x eip=%x\n", tf->tf_esp, tf->tf_eip));
#ifdef VM86
	if (scp->sc_eflags & PSL_VM) {
		void syscall_vm86 __P((struct trapframe *));

		tf->tf_vm86_gs = scp->sc_gs;
		tf->tf_vm86_fs = scp->sc_fs;
		tf->tf_vm86_es = scp->sc_es;
		tf->tf_vm86_ds = scp->sc_ds;
		set_vflags(l, scp->sc_eflags);
		p->p_md.md_syscall = syscall_vm86;
	} else
#endif
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((scp->sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(scp->sc_cs, scp->sc_eflags))
			return EINVAL;

		tf->tf_gs = scp->sc_gs;
		tf->tf_fs = scp->sc_fs;
		tf->tf_es = scp->sc_es;
		tf->tf_ds = scp->sc_ds;
#ifdef VM86
		if (tf->tf_eflags & PSL_VM)
			(*p->p_emul->e_syscall_intern)(p);
#endif
		tf->tf_eflags = scp->sc_eflags;
	}
	tf->tf_edi = scp->sc_edi;
	tf->tf_esi = scp->sc_esi;
	tf->tf_ebp = scp->sc_ebp;
	tf->tf_ebx = scp->sc_ebx;
	tf->tf_edx = scp->sc_edx;
	tf->tf_ecx = scp->sc_ecx;
	tf->tf_eax = scp->sc_eax;
	tf->tf_eip = scp->sc_eip;
	tf->tf_cs = scp->sc_cs;
	tf->tf_esp = scp->sc_esp_at_signal;
	tf->tf_ss = scp->sc_ss;

	/* Restore signal stack. */
	/*
	 * Linux really does it this way; it doesn't have space in sigframe
	 * to save the onstack flag.
	 */
	ss_gap = (ssize_t)
	    ((caddr_t) scp->sc_esp_at_signal - (caddr_t) sas->ss_sp);
	if (ss_gap >= 0 && ss_gap < sas->ss_size)
		sas->ss_flags |= SS_ONSTACK;
	else
		sas->ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux_old_to_native_sigset(&mask, &scp->sc_mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);
	DPRINTF(("sigreturn exit esp=%x eip=%x\n", tf->tf_esp, tf->tf_eip));
	return EJUSTRETURN;
}

#ifdef USER_LDT

int
linux_read_ldt(l, uap, retval)
	struct lwp *l;
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct i386_get_ldt_args gl;
	int error;
	caddr_t sg;
	char *parms;

	DPRINTF(("linux_read_ldt!"));
	sg = stackgap_init(p, 0);

	gl.start = 0;
	gl.desc = SCARG(uap, ptr);
	gl.num = SCARG(uap, bytecount) / sizeof(union descriptor);

	parms = stackgap_alloc(p, &sg, sizeof(gl));

	if ((error = copyout(&gl, parms, sizeof(gl))) != 0)
		return (error);

	if ((error = i386_get_ldt(l, parms, retval)) != 0)
		return (error);

	*retval *= sizeof(union descriptor);
	return (0);
}

struct linux_ldt_info {
	u_int entry_number;
	u_long base_addr;
	u_int limit;
	u_int seg_32bit:1;
	u_int contents:2;
	u_int read_exec_only:1;
	u_int limit_in_pages:1;
	u_int seg_not_present:1;
	u_int useable:1;
};

int
linux_write_ldt(l, uap, retval)
	struct lwp *l;
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct linux_ldt_info ldt_info;
	struct segment_descriptor sd;
	struct i386_set_ldt_args sl;
	int error;
	caddr_t sg;
	char *parms;
	int oldmode = (int)retval[0];

	DPRINTF(("linux_write_ldt %d\n", oldmode));
	if (SCARG(uap, bytecount) != sizeof(ldt_info))
		return (EINVAL);
	if ((error = copyin(SCARG(uap, ptr), &ldt_info, sizeof(ldt_info))) != 0)
		return error;
	if (ldt_info.entry_number >= 8192)
		return (EINVAL);
	if (ldt_info.contents == 3) {
		if (oldmode)
			return (EINVAL);
		if (ldt_info.seg_not_present)
			return (EINVAL);
	}

	if (ldt_info.base_addr == 0 && ldt_info.limit == 0 &&
	    (oldmode || (ldt_info.contents == 0 &&
	    ldt_info.read_exec_only == 1 && ldt_info.seg_32bit == 0 &&
	    ldt_info.limit_in_pages == 0 && ldt_info.seg_not_present == 1 &&
	    ldt_info.useable == 0))) {
		/* this means you should zero the ldt */
		(void)memset(&sd, 0, sizeof(sd));
	} else {
		sd.sd_lobase = ldt_info.base_addr & 0xffffff;
		sd.sd_hibase = (ldt_info.base_addr >> 24) & 0xff;
		sd.sd_lolimit = ldt_info.limit & 0xffff;
		sd.sd_hilimit = (ldt_info.limit >> 16) & 0xf;
		sd.sd_type = 16 | (ldt_info.contents << 2) |
		    (!ldt_info.read_exec_only << 1);
		sd.sd_dpl = SEL_UPL;
		sd.sd_p = !ldt_info.seg_not_present;
		sd.sd_def32 = ldt_info.seg_32bit;
		sd.sd_gran = ldt_info.limit_in_pages;
		if (!oldmode)
			sd.sd_xx = ldt_info.useable;
		else
			sd.sd_xx = 0;
	}
	sg = stackgap_init(p, 0);
	sl.start = ldt_info.entry_number;
	sl.desc = stackgap_alloc(p, &sg, sizeof(sd));
	sl.num = 1;

	DPRINTF(("linux_write_ldt: idx=%d, base=0x%lx, limit=0x%x\n",
	    ldt_info.entry_number, ldt_info.base_addr, ldt_info.limit));

	parms = stackgap_alloc(p, &sg, sizeof(sl));

	if ((error = copyout(&sd, sl.desc, sizeof(sd))) != 0)
		return (error);
	if ((error = copyout(&sl, parms, sizeof(sl))) != 0)
		return (error);

	if ((error = i386_set_ldt(l, parms, retval)) != 0)
		return (error);

	*retval = 0;
	return (0);
}

#endif /* USER_LDT */

int
linux_sys_modify_ldt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap = v;

	switch (SCARG(uap, func)) {
#ifdef USER_LDT
	case 0:
		return linux_read_ldt(l, uap, retval);
	case 1:
		retval[0] = 1;
		return linux_write_ldt(l, uap, retval);
	case 2:
#ifdef notyet
		return (linux_read_default_ldt(l, uap, retval);
#else
		return (ENOSYS);
#endif
	case 0x11:
		retval[0] = 0;
		return linux_write_ldt(l, uap, retval);
#endif /* USER_LDT */

	default:
		return (ENOSYS);
	}
}

/*
 * XXX Pathetic hack to make svgalib work. This will fake the major
 * device number of an opened VT so that svgalib likes it. grmbl.
 * Should probably do it 'wrong the right way' and use a mapping
 * array for all major device numbers, and map linux_mknod too.
 */
dev_t
linux_fakedev(dev, raw)
	dev_t dev;
	int raw;
{
	if (raw) {
#if (NWSDISPLAY > 0)
		extern const struct cdevsw wsdisplay_cdevsw;
		if (cdevsw_lookup(dev) == &wsdisplay_cdevsw)
			return makedev(LINUX_CONS_MAJOR, (minor(dev) + 1));
#endif
	}

	return dev;
}

#if (NWSDISPLAY > 0)
/*
 * That's not complete, but enough to get an X server running.
 */
#define NR_KEYS 128
static const u_short plain_map[NR_KEYS] = {
	0x0200,	0x001b,	0x0031,	0x0032,	0x0033,	0x0034,	0x0035,	0x0036,
	0x0037,	0x0038,	0x0039,	0x0030,	0x002d,	0x003d,	0x007f,	0x0009,
	0x0b71,	0x0b77,	0x0b65,	0x0b72,	0x0b74,	0x0b79,	0x0b75,	0x0b69,
	0x0b6f,	0x0b70,	0x005b,	0x005d,	0x0201,	0x0702,	0x0b61,	0x0b73,
	0x0b64,	0x0b66,	0x0b67,	0x0b68,	0x0b6a,	0x0b6b,	0x0b6c,	0x003b,
	0x0027,	0x0060,	0x0700,	0x005c,	0x0b7a,	0x0b78,	0x0b63,	0x0b76,
	0x0b62,	0x0b6e,	0x0b6d,	0x002c,	0x002e,	0x002f,	0x0700,	0x030c,
	0x0703,	0x0020,	0x0207,	0x0100,	0x0101,	0x0102,	0x0103,	0x0104,
	0x0105,	0x0106,	0x0107,	0x0108,	0x0109,	0x0208,	0x0209,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x003c,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x001c,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, shift_map[NR_KEYS] = {
	0x0200,	0x001b,	0x0021,	0x0040,	0x0023,	0x0024,	0x0025,	0x005e,
	0x0026,	0x002a,	0x0028,	0x0029,	0x005f,	0x002b,	0x007f,	0x0009,
	0x0b51,	0x0b57,	0x0b45,	0x0b52,	0x0b54,	0x0b59,	0x0b55,	0x0b49,
	0x0b4f,	0x0b50,	0x007b,	0x007d,	0x0201,	0x0702,	0x0b41,	0x0b53,
	0x0b44,	0x0b46,	0x0b47,	0x0b48,	0x0b4a,	0x0b4b,	0x0b4c,	0x003a,
	0x0022,	0x007e,	0x0700,	0x007c,	0x0b5a,	0x0b58,	0x0b43,	0x0b56,
	0x0b42,	0x0b4e,	0x0b4d,	0x003c,	0x003e,	0x003f,	0x0700,	0x030c,
	0x0703,	0x0020,	0x0207,	0x010a,	0x010b,	0x010c,	0x010d,	0x010e,
	0x010f,	0x0110,	0x0111,	0x0112,	0x0113,	0x0213,	0x0203,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x003e,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x0200,	0x0701,	0x0205,	0x0114,	0x0603,
	0x020b,	0x0601,	0x0602,	0x0117,	0x0600,	0x020a,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, altgr_map[NR_KEYS] = {
	0x0200,	0x0200,	0x0200,	0x0040,	0x0200,	0x0024,	0x0200,	0x0200,
	0x007b,	0x005b,	0x005d,	0x007d,	0x005c,	0x0200,	0x0200,	0x0200,
	0x0b71,	0x0b77,	0x0918,	0x0b72,	0x0b74,	0x0b79,	0x0b75,	0x0b69,
	0x0b6f,	0x0b70,	0x0200,	0x007e,	0x0201,	0x0702,	0x0914,	0x0b73,
	0x0917,	0x0919,	0x0b67,	0x0b68,	0x0b6a,	0x0b6b,	0x0b6c,	0x0200,
	0x0200,	0x0200,	0x0700,	0x0200,	0x0b7a,	0x0b78,	0x0916,	0x0b76,
	0x0915,	0x0b6e,	0x0b6d,	0x0200,	0x0200,	0x0200,	0x0700,	0x030c,
	0x0703,	0x0200,	0x0207,	0x050c,	0x050d,	0x050e,	0x050f,	0x0510,
	0x0511,	0x0512,	0x0513,	0x0514,	0x0515,	0x0208,	0x0202,	0x0911,
	0x0912,	0x0913,	0x030b,	0x090e,	0x090f,	0x0910,	0x030a,	0x090b,
	0x090c,	0x090d,	0x090a,	0x0310,	0x0206,	0x0200,	0x007c,	0x0516,
	0x0517,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x0200,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, ctrl_map[NR_KEYS] = {
	0x0200,	0x0200,	0x0200,	0x0000,	0x001b,	0x001c,	0x001d,	0x001e,
	0x001f,	0x007f,	0x0200,	0x0200,	0x001f,	0x0200,	0x0008,	0x0200,
	0x0011,	0x0017,	0x0005,	0x0012,	0x0014,	0x0019,	0x0015,	0x0009,
	0x000f,	0x0010,	0x001b,	0x001d,	0x0201,	0x0702,	0x0001,	0x0013,
	0x0004,	0x0006,	0x0007,	0x0008,	0x000a,	0x000b,	0x000c,	0x0200,
	0x0007,	0x0000,	0x0700,	0x001c,	0x001a,	0x0018,	0x0003,	0x0016,
	0x0002,	0x000e,	0x000d,	0x0200,	0x020e,	0x007f,	0x0700,	0x030c,
	0x0703,	0x0000,	0x0207,	0x0100,	0x0101,	0x0102,	0x0103,	0x0104,
	0x0105,	0x0106,	0x0107,	0x0108,	0x0109,	0x0208,	0x0204,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x0200,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x001c,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
};

const u_short * const linux_keytabs[] = {
	plain_map, shift_map, altgr_map, altgr_map, ctrl_map
};
#endif

static struct biosdisk_info *
fd2biosinfo(p, fp)
	struct proc *p;
	struct file *fp;
{
	struct vnode *vp;
	const char *blkname;
	char diskname[16];
	int i;
	struct nativedisk_info *nip;
	struct disklist *dl = i386_alldisks;

	if (fp->f_type != DTYPE_VNODE)
		return NULL;
	vp = (struct vnode *)fp->f_data;

	if (vp->v_type != VBLK)
		return NULL;

	blkname = devsw_blk2name(major(vp->v_rdev));
	snprintf(diskname, sizeof diskname, "%s%u", blkname,
	    DISKUNIT(vp->v_rdev));

	for (i = 0; i < dl->dl_nnativedisks; i++) {
		nip = &dl->dl_nativedisks[i];
		if (strcmp(diskname, nip->ni_devname))
			continue;
		if (nip->ni_nmatches != 0)
			return &dl->dl_biosdisks[nip->ni_biosmatches[0]];
	}

	return NULL;
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
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	struct sys_ioctl_args bia;
	u_long com;
	int error, error1;
#if (NWSDISPLAY > 0)
	struct vt_mode lvt;
	caddr_t bvtp, sg;
	struct kbentry kbe;
#endif
	struct linux_hd_geometry hdg;
	struct linux_hd_big_geometry hdg_big;
	struct biosdisk_info *bip;
	struct filedesc *fdp;
	struct file *fp;
	int fd;
	struct disklabel label, *labp;
	struct partinfo partp;
	int (*ioctlf)(struct file *, u_long, void *, struct proc *);
	u_long start, biostotal, realtotal;
	u_char heads, sectors;
	u_int cylinders;
	struct ioctl_pt pt;

	fd = SCARG(uap, fd);
	SCARG(&bia, fd) = fd;
	SCARG(&bia, data) = SCARG(uap, data);
	com = SCARG(uap, com);

	fdp = p->p_fd;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);

	switch (com) {
#if (NWSDISPLAY > 0)
	case LINUX_KDGKBMODE:
		com = KDGKBMODE;
		break;
	case LINUX_KDSKBMODE:
		com = KDSKBMODE;
		if ((unsigned)SCARG(uap, data) == LINUX_K_MEDIUMRAW)
			SCARG(&bia, data) = (caddr_t)K_RAW;
		break;
	case LINUX_KIOCSOUND:
		SCARG(&bia, data) =
		    (caddr_t)(((unsigned long)SCARG(&bia, data)) & 0xffff);
		/* fall through */
	case LINUX_KDMKTONE:
		com = KDMKTONE;
		break;
	case LINUX_KDSETMODE:
		com = KDSETMODE;
		break;
	case LINUX_KDGETMODE:
		/* KD_* values are equal to the wscons numbers */
		com = WSDISPLAYIO_GMODE;
		break;
	case LINUX_KDENABIO:
		com = KDENABIO;
		break;
	case LINUX_KDDISABIO:
		com = KDDISABIO;
		break;
	case LINUX_KDGETLED:
		com = KDGETLED;
		break;
	case LINUX_KDSETLED:
		com = KDSETLED;
		break;
	case LINUX_VT_OPENQRY:
		com = VT_OPENQRY;
		break;
	case LINUX_VT_GETMODE:
		SCARG(&bia, com) = VT_GETMODE;
		/* XXX NJWLWP */
		if ((error = sys_ioctl(curlwp, &bia, retval)))
			goto out;
		if ((error = copyin(SCARG(uap, data), (caddr_t)&lvt,
		    sizeof (struct vt_mode))))
			goto out;
		lvt.relsig = native_to_linux_signo[lvt.relsig];
		lvt.acqsig = native_to_linux_signo[lvt.acqsig];
		lvt.frsig = native_to_linux_signo[lvt.frsig];
		error = copyout((caddr_t)&lvt, SCARG(uap, data),
		    sizeof (struct vt_mode));
		goto out;
	case LINUX_VT_SETMODE:
		com = VT_SETMODE;
		if ((error = copyin(SCARG(uap, data), (caddr_t)&lvt,
		    sizeof (struct vt_mode))))
			goto out;
		lvt.relsig = linux_to_native_signo[lvt.relsig];
		lvt.acqsig = linux_to_native_signo[lvt.acqsig];
		lvt.frsig = linux_to_native_signo[lvt.frsig];
		sg = stackgap_init(p, 0);
		bvtp = stackgap_alloc(p, &sg, sizeof (struct vt_mode));
		if ((error = copyout(&lvt, bvtp, sizeof (struct vt_mode))))
			goto out;
		SCARG(&bia, data) = bvtp;
		break;
	case LINUX_VT_DISALLOCATE:
		/* XXX should use WSDISPLAYIO_DELSCREEN */
		error = 0;
		goto out;
	case LINUX_VT_RELDISP:
		com = VT_RELDISP;
		break;
	case LINUX_VT_ACTIVATE:
		com = VT_ACTIVATE;
		break;
	case LINUX_VT_WAITACTIVE:
		com = VT_WAITACTIVE;
		break;
	case LINUX_VT_GETSTATE:
		com = VT_GETSTATE;
		break;
	case LINUX_KDGKBTYPE:
	    {
		static const u_int8_t kb101 = KB_101;

		/* This is what Linux does. */
		error = copyout(&kb101, SCARG(uap, data), 1);
		goto out;
	    }
	case LINUX_KDGKBENT:
		/*
		 * The Linux KDGKBENT ioctl is different from the
		 * SYSV original. So we handle it in machdep code.
		 * XXX We should use keyboard mapping information
		 * from wsdisplay, but this would be expensive.
		 */
		if ((error = copyin(SCARG(uap, data), &kbe,
				    sizeof(struct kbentry))))
			goto out;
		if (kbe.kb_table >= sizeof(linux_keytabs) / sizeof(u_short *)
		    || kbe.kb_index >= NR_KEYS) {
			error = EINVAL;
			goto out;
		}
		kbe.kb_value = linux_keytabs[kbe.kb_table][kbe.kb_index];
		error = copyout(&kbe, SCARG(uap, data),
				sizeof(struct kbentry));
		goto out;
#endif
	case LINUX_HDIO_GETGEO:
	case LINUX_HDIO_GETGEO_BIG:
		/*
		 * Try to mimic Linux behaviour: return the BIOS geometry
		 * if possible (extending its # of cylinders if it's beyond
		 * the 1023 limit), fall back to the MI geometry (i.e.
		 * the real geometry) if not found, by returning an
		 * error. See common/linux_hdio.c
		 */
		bip = fd2biosinfo(p, fp);
		ioctlf = fp->f_ops->fo_ioctl;
		error = ioctlf(fp, DIOCGDEFLABEL, (caddr_t)&label, p);
		error1 = ioctlf(fp, DIOCGPART, (caddr_t)&partp, p);
		if (error != 0 && error1 != 0) {
			error = error1;
			goto out;
		}
		labp = error != 0 ? &label : partp.disklab;
		start = error1 != 0 ? partp.part->p_offset : 0;
		if (bip != NULL && bip->bi_head != 0 && bip->bi_sec != 0
		    && bip->bi_cyl != 0) {
			heads = bip->bi_head;
			sectors = bip->bi_sec;
			cylinders = bip->bi_cyl;
			biostotal = heads * sectors * cylinders;
			realtotal = labp->d_ntracks * labp->d_nsectors *
			    labp->d_ncylinders;
			if (realtotal > biostotal)
				cylinders = realtotal / (heads * sectors);
		} else {
			heads = labp->d_ntracks;
			cylinders = labp->d_ncylinders;
			sectors = labp->d_nsectors;
		}
		if (com == LINUX_HDIO_GETGEO) {
			hdg.start = start;
			hdg.heads = heads;
			hdg.cylinders = cylinders;
			hdg.sectors = sectors;
			error = copyout(&hdg, SCARG(uap, data), sizeof hdg);
			goto out;
		} else {
			hdg_big.start = start;
			hdg_big.heads = heads;
			hdg_big.cylinders = cylinders;
			hdg_big.sectors = sectors;
			error = copyout(&hdg_big, SCARG(uap, data),
			    sizeof hdg_big);
			goto out;
		}

	default:
		/*
		 * Unknown to us. If it's on a device, just pass it through
		 * using PTIOCLINUX, the device itself might be able to
		 * make some sense of it.
		 * XXX hack: if the function returns EJUSTRETURN,
		 * it has stuffed a sysctl return value in pt.data.
		 */
		FILE_USE(fp);
		ioctlf = fp->f_ops->fo_ioctl;
		pt.com = SCARG(uap, com);
		pt.data = SCARG(uap, data);
		error = ioctlf(fp, PTIOCLINUX, (caddr_t)&pt, p);
		FILE_UNUSE(fp, p);
		if (error == EJUSTRETURN) {
			retval[0] = (register_t)pt.data;
			error = 0;
		}

		if (error == ENOTTY)
			DPRINTF(("linux_machdepioctl: invalid ioctl %08lx\n",
			    com));
		goto out;
	}
	SCARG(&bia, com) = com;
	/* XXX NJWLWP */
	error = sys_ioctl(curlwp, &bia, retval);
out:
	FILE_UNUSE(fp ,p);
	return error;
}

/*
 * Set I/O permissions for a process. Just set the maximum level
 * right away (ignoring the argument), otherwise we would have
 * to rely on I/O permission maps, which are not implemented.
 */
int
linux_sys_iopl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#if 0
	struct linux_sys_iopl_args /* {
		syscallarg(int) level;
	} */ *uap = v;
#endif
	struct proc *p = l->l_proc;
	struct trapframe *fp = l->l_md.md_regs;

	if (suser(p->p_ucred, &p->p_acflag) != 0)
		return EPERM;
	fp->tf_eflags |= PSL_IOPL;
	*retval = 0;
	return 0;
}

/*
 * See above. If a root process tries to set access to an I/O port,
 * just let it have the whole range.
 */
int
linux_sys_ioperm(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_ioperm_args /* {
		syscallarg(unsigned int) lo;
		syscallarg(unsigned int) hi;
		syscallarg(int) val;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct trapframe *fp = l->l_md.md_regs;

	if (suser(p->p_ucred, &p->p_acflag) != 0)
		return EPERM;
	if (SCARG(uap, val))
		fp->tf_eflags |= PSL_IOPL;
	*retval = 0;
	return 0;
}

int
linux_exec_setup_stack(struct proc *p, struct exec_package *epp)
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

#ifndef	USRSTACK32
#define USRSTACK32	(0x00000000ffffffffL&~PGOFSET)
#endif

	if (epp->ep_flags & EXEC_32) {
		epp->ep_minsaddr = USRSTACK32;
		max_stack_size = MAXSSIZ;
	} else {
		epp->ep_minsaddr = USRSTACK;
		max_stack_size = MAXSSIZ;
	}

	if (epp->ep_minsaddr > LINUX_USRSTACK)
		epp->ep_minsaddr = LINUX_USRSTACK;
#ifdef DEBUG_LINUX
	else {
		/*
		 * Someone needs to make KERNBASE and TEXTADDR
		 * java versions < 1.4.2 need the stack to be
		 * at 0xC0000000
		 */
		uprintf("Cannot setup stack to 0xC0000000, "
		    "java will not work properly\n");
	}
#endif
	epp->ep_maxsaddr = (u_long)STACK_GROW(epp->ep_minsaddr, 
		max_stack_size);
	epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 */
	access_size = epp->ep_ssize;
	access_linear_min = (u_long)STACK_ALLOC(epp->ep_minsaddr, access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long)STACK_ALLOC(STACK_GROW(epp->ep_minsaddr, 
	    access_size), noaccess_size);
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
	    noaccess_linear_min, NULLVP, 0, VM_PROT_NONE);
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return 0;
}
