/*	$NetBSD: compat_16_machdep.c,v 1.3 2003/09/11 19:15:12 christos Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: compat_16_machdep.c,v 1.3 2003/09/11 19:15:12 christos Exp $");

#include "opt_vm86.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#ifdef VM86
#include <machine/mcontext.h>
#include <machine/vm86.h>
#endif
#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/pmap.h>
#include <machine/vmparam.h>

#if defined(COMPAT_16) || defined(COMPAT_IBCS2)
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
int compat_16_sys___sigreturn14(struct lwp *, void *, register_t *);

int
compat_16_sys___sigreturn14(struct lwp *l, void *v, register_t *retval)
{
	struct compat_16_sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore register context. */
	tf = l->l_md.md_regs;
#ifdef VM86
	if (context.sc_eflags & PSL_VM) {
		void syscall_vm86 __P((struct trapframe *));

		tf->tf_vm86_gs = context.sc_gs;
		tf->tf_vm86_fs = context.sc_fs;
		tf->tf_vm86_es = context.sc_es;
		tf->tf_vm86_ds = context.sc_ds;
		set_vflags(l, context.sc_eflags);
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
		if (((context.sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(context.sc_cs, context.sc_eflags))
			return (EINVAL);

		tf->tf_gs = context.sc_gs;
		tf->tf_fs = context.sc_fs;
		tf->tf_es = context.sc_es;
		tf->tf_ds = context.sc_ds;
		tf->tf_eflags = context.sc_eflags;
	}
	tf->tf_edi = context.sc_edi;
	tf->tf_esi = context.sc_esi;
	tf->tf_ebp = context.sc_ebp;
	tf->tf_ebx = context.sc_ebx;
	tf->tf_edx = context.sc_edx;
	tf->tf_ecx = context.sc_ecx;
	tf->tf_eax = context.sc_eax;
	tf->tf_eip = context.sc_eip;
	tf->tf_cs = context.sc_cs;
	tf->tf_esp = context.sc_esp;
	tf->tf_ss = context.sc_ss;

	/* Restore signal stack. */
	if (context.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &context.sc_mask, 0);

	return (EJUSTRETURN);
}
#endif

#ifdef COMPAT_16
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
sendsig_sigcontext(ksiginfo_t *ksi, sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pmap *pmap = vm_map_pmap(&p->p_vmspace->vm_map);
	int sel = pmap->pm_hiexec > I386_MAX_EXE_ADDR ?
	    GUCODEBIG_SEL : GUCODE_SEL;
	struct sigacts *ps = p->p_sigacts;
	struct trapframe *tf = l->l_md.md_regs;
	int onstack;
	int sig = ksi->ksi_signo;
	u_long code = ksi->ksi_trap;
	struct sigframe_sigcontext *fp = getframe(l, sig, &onstack), frame;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	fp--;

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* legacy on-stack sigtramp */
		frame.sf_ra = (int)p->p_sigctx.ps_sigcode;
		break;

	case 1:
		frame.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
		break;

	default:
		/* Don't know what trampoline version; kill it. */
		sigexit(l, SIGILL);
	}

	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = &fp->sf_sc;

	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		frame.sf_sc.sc_eflags = get_vflags(l);
		(*p->p_emul->e_syscall_intern)(p);
	} else
#endif
	{
		frame.sf_sc.sc_gs = tf->tf_gs;
		frame.sf_sc.sc_fs = tf->tf_fs;
		frame.sf_sc.sc_es = tf->tf_es;
		frame.sf_sc.sc_ds = tf->tf_ds;
		frame.sf_sc.sc_eflags = tf->tf_eflags;
	}
	frame.sf_sc.sc_edi = tf->tf_edi;
	frame.sf_sc.sc_esi = tf->tf_esi;
	frame.sf_sc.sc_ebp = tf->tf_ebp;
	frame.sf_sc.sc_ebx = tf->tf_ebx;
	frame.sf_sc.sc_edx = tf->tf_edx;
	frame.sf_sc.sc_ecx = tf->tf_ecx;
	frame.sf_sc.sc_eax = tf->tf_eax;
	frame.sf_sc.sc_eip = tf->tf_eip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp = tf->tf_esp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
	frame.sf_sc.sc_err = tf->tf_err;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

#ifdef COMPAT_13
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &frame.sf_sc.__sc_mask13);
#endif

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, sel, catcher, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}
#endif

#if defined(COMPAT_16) && defined(VM86)
struct compat_16_vm86_kern {
	struct sigcontext regs;
	unsigned long ss_cpu_type;
};

struct compat_16_vm86_struct {
	struct compat_16_vm86_kern substr;
	unsigned long screen_bitmap;	/* not used/supported (yet) */
	unsigned long flags;		/* not used/supported (yet) */
	unsigned char int_byuser[32];	/* 256 bits each: pass control to user */
	unsigned char int21_byuser[32];	/* otherwise, handle directly */
};

int
compat_16_i386_vm86(struct lwp *l, char *args, register_t *retval)
{
	struct trapframe *tf = l->l_md.md_regs;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct compat_16_vm86_kern vm86s;
	int error;

	error = copyin(args, &vm86s, sizeof(vm86s));
	if (error)
		return (error);

	pcb->vm86_userp = (void *)(args +
	    (offsetof(struct compat_16_vm86_struct, screen_bitmap)
	    - offsetof(struct vm86_struct, screen_bitmap)));
	printf("offsetting by %lu\n", (unsigned long)
	    (offsetof(struct compat_16_vm86_struct, screen_bitmap)
	    - offsetof(struct vm86_struct, screen_bitmap)));

	/*
	 * Keep mask of flags we simulate to simulate a particular type of
	 * processor.
	 */
	switch (vm86s.ss_cpu_type) {
	case VCPU_086:
	case VCPU_186:
	case VCPU_286:
		pcb->vm86_flagmask = PSL_ID|PSL_AC|PSL_NT|PSL_IOPL;
		break;
	case VCPU_386:
		pcb->vm86_flagmask = PSL_ID|PSL_AC;
		break;
	case VCPU_486:
		pcb->vm86_flagmask = PSL_ID;
		break;
	case VCPU_586:
		pcb->vm86_flagmask = 0;
		break;
	default:
		return (EINVAL);
	}

#define DOVREG(reg) tf->tf_vm86_##reg = (u_short) vm86s.regs.sc_##reg
#define DOREG(reg) tf->tf_##reg = (u_short) vm86s.regs.sc_##reg

	DOVREG(ds);
	DOVREG(es);
	DOVREG(fs);
	DOVREG(gs);
	DOREG(edi);
	DOREG(esi);
	DOREG(ebp);
	DOREG(eax);
	DOREG(ebx);
	DOREG(ecx);
	DOREG(edx);
	DOREG(eip);
	DOREG(cs);
	DOREG(esp);
	DOREG(ss);

#undef	DOVREG
#undef	DOREG

	/* Going into vm86 mode jumps off the signal stack. */
	l->l_proc->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	set_vflags(l, vm86s.regs.sc_eflags | PSL_VM);

	return (EJUSTRETURN);
}

#endif

