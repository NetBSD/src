/*	$NetBSD: netbsd32_machdep.c,v 1.3 2002/05/28 23:11:39 fvdl Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/map.h>

#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/vmparam.h>
#include <machine/netbsd32_machdep.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int process_read_fpregs32(struct proc *, struct fpreg32 *);
int process_read_regs32(struct proc *, struct reg32 *);

void
netbsd32_setregs(struct proc *p, struct exec_package *pack, u_long stack)
{
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct trapframe *tf;

	/* If we were using the FPU, forget about it. */
	if (fpuproc == p)
		fpudrop();

#if defined(USER_LDT) && 0
	pmap_ldt_cleanup(p);
#endif

	p->p_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;
	pcb->pcb_savefpu.fx_fcw = __NetBSD_NPXCW__;

	p->p_flag |= P_32;

	tf = p->p_md.md_regs;
	__asm("movl %0,%%gs" : : "r" (LSEL(LUDATA32_SEL, SEL_UPL)));
	__asm("movl %0,%%fs" : : "r" (LSEL(LUDATA32_SEL, SEL_UPL)));

	/*
	 * XXXfvdl needs to be revisited
	 * if USER_LDT is going to be supported, these need
	 * to be saved/restored.
	 */
#if 1
	__asm("movl %0,%%ds" : : "r" (LSEL(LUDATA32_SEL, SEL_UPL)));
	__asm("movl %0,%%es" : : "r" (LSEL(LUDATA32_SEL, SEL_UPL)));
#else
	tf->tf_es = LSEL(LUDATA32_SEL, SEL_UPL);
	tf->tf_ds = LSEL(LUDATA32_SEL, SEL_UPL);
#endif
	tf->tf_rdi = 0;
	tf->tf_rsi = 0;
	tf->tf_rbp = 0;
	tf->tf_rbx = (u_int64_t)p->p_psstr;
	tf->tf_rdx = 0;
	tf->tf_rcx = 0;
	tf->tf_rax = 0;
	tf->tf_rip = pack->ep_entry;
	tf->tf_cs = LSEL(LUCODE32_SEL, SEL_UPL);
	tf->tf_rflags = PSL_USERSET;
	tf->tf_rsp = stack;
	tf->tf_ss = LSEL(LUDATA32_SEL, SEL_UPL);
}

void
netbsd32_sendsig(sig_t catcher, int sig, sigset_t *mask, u_long code)
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct netbsd32_sigframe *fp, frame;
	int onstack;

	tf = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct netbsd32_sigframe *)
		    ((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
					  p->p_sigctx.ps_sigstk.ss_size);
	else
		fp = (struct netbsd32_sigframe *)tf->tf_rsp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_signum = sig;
	frame.sf_code = code;
	frame.sf_scp = (u_int32_t)(u_long)&fp->sf_sc;
	frame.sf_handler = (u_int32_t)(u_long)catcher;

	/*
	 * XXXfvdl these need to be saved and restored for USER_LDT.
	 */

	/* Save register context. */
	__asm("movl %%gs,%0" : "=r" (frame.sf_sc.sc_gs));
	__asm("movl %%fs,%0" : "=r" (frame.sf_sc.sc_fs));
#if 1
	frame.sf_sc.sc_es = LSEL(LUDATA32_SEL, SEL_UPL);
	frame.sf_sc.sc_ds = LSEL(LUDATA32_SEL, SEL_UPL);
#else
	frame.sf_sc.sc_es = tf->tf_es;
	frame.sf_sc.sc_ds = tf->tf_ds;
#endif
	frame.sf_sc.sc_eflags = tf->tf_rflags;
	frame.sf_sc.sc_edi = tf->tf_rdi;
	frame.sf_sc.sc_esi = tf->tf_rsi;
	frame.sf_sc.sc_ebp = tf->tf_rbp;
	frame.sf_sc.sc_ebx = tf->tf_rbx;
	frame.sf_sc.sc_edx = tf->tf_rdx;
	frame.sf_sc.sc_ecx = tf->tf_rcx;
	frame.sf_sc.sc_eax = tf->tf_rax;
	frame.sf_sc.sc_eip = tf->tf_rip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp = tf->tf_rsp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
	frame.sf_sc.sc_err = tf->tf_err;

	/* Save signal stack. */
	frame.sf_sc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	frame.sf_sc.sc_mask = *mask;

	if (copyout(&frame, fp, sizeof frame) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	__asm("movl %0,%%gs" : : "r" (GSEL(GUDATA32_SEL, SEL_UPL)));
	__asm("movl %0,%%fs" : : "r" (GSEL(GUDATA32_SEL, SEL_UPL)));
#if 1
	/* XXXX */
	__asm("movl %0,%%es" : : "r" (GSEL(GUDATA32_SEL, SEL_UPL)));
	__asm("movl %0,%%ds" : : "r" (GSEL(GUDATA32_SEL, SEL_UPL)));
#else
	tf->tf_es = GSEL(GUDATA32_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA32_SEL, SEL_UPL);
#endif
	tf->tf_rip = (u_int64_t)p->p_sigctx.ps_sigcode;
	tf->tf_cs = GSEL(GUCODE32_SEL, SEL_UPL);
	tf->tf_rflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_rsp = (u_int64_t)fp;
	tf->tf_ss = GSEL(GUDATA32_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}


int
netbsd32___sigreturn14(struct proc *p, void *v, register_t *retval)
{
	struct netbsd32___sigreturn14_args /* {
		syscallarg(struct netbsd32_sigcontext *) sigcntxp;
	} */ *uap = v;
	struct netbsd32_sigcontext *scp, context;
	struct trapframe *tf;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = (struct netbsd32_sigcontext *)(unsigned long)SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore register context. */
	tf = p->p_md.md_regs;
	/*
	 * Check for security violations.  If we're returning to
	 * protected mode, the CPU will validate the segment registers
	 * automatically and generate a trap on violations.  We handle
	 * the trap, rather than doing all of the checking here.
	 */
	if (((context.sc_eflags ^ tf->tf_rflags) & PSL_USERSTATIC) != 0 ||
	    !USERMODE(context.sc_cs, context.sc_eflags))
		return (EINVAL);

	/* %fs and %gs were restored by the trampoline. */
#if 1
	__asm("movl %0,%%ds" : : "r" (context.sc_ds));
	__asm("movl %0,%%es" : : "r" (context.sc_es));
#else
	tf->tf_es = context.sc_es;
	tf->tf_ds = context.sc_ds;
#endif
	tf->tf_rflags = context.sc_eflags;
	tf->tf_rdi = context.sc_edi;
	tf->tf_rsi = context.sc_esi;
	tf->tf_rbp = context.sc_ebp;
	tf->tf_rbx = context.sc_ebx;
	tf->tf_rdx = context.sc_edx;
	tf->tf_rcx = context.sc_ecx;
	tf->tf_rax = context.sc_eax;
	tf->tf_rip = context.sc_eip;
	tf->tf_cs = context.sc_cs;
	tf->tf_rsp = context.sc_esp;
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


/*
 * Dump the machine specific segment at the start of a core dump.
 */     
struct md_core32 {
	struct reg32 intreg;
	struct fpreg32 freg;
};

int
cpu_coredump32(struct proc *p, struct vnode *vp, struct ucred *cred,
	     struct core32 *chdr)
{
	struct md_core32 md_core;
	struct coreseg cseg;
	int error;

	CORE_SETMAGIC(*chdr, COREMAGIC, MID_MACHINE, 0);
	chdr->c_hdrsize = ALIGN32(sizeof(*chdr));
	chdr->c_seghdrsize = ALIGN32(sizeof(cseg));
	chdr->c_cpusize = sizeof(md_core);

	/* Save integer registers. */
	error = process_read_regs32(p, &md_core.intreg);
	if (error)
		return error;

	/* Save floating point registers. */
	error = process_read_fpregs32(p, &md_core.freg);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_I386, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cseg, chdr->c_seghdrsize,
	    (off_t)chdr->c_hdrsize, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred,
	    NULL, p);
	if (error)
		return error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&md_core, sizeof(md_core),
	    (off_t)(chdr->c_hdrsize + chdr->c_seghdrsize), UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return error;

	chdr->c_nseg++;
	return 0;
}


int
process_read_regs32(struct proc *p, struct reg32 *regs)
{
	struct trapframe *tf = p->p_md.md_regs;

	regs->r_gs = LSEL(LUCODE32_SEL, SEL_UPL);
	regs->r_fs = LSEL(LUCODE32_SEL, SEL_UPL);
	regs->r_es = LSEL(LUCODE32_SEL, SEL_UPL);
	regs->r_ds = LSEL(LUCODE32_SEL, SEL_UPL);
	regs->r_eflags = tf->tf_rflags;
	/* XXX avoid sign extension problems with unknown upper bits? */
	regs->r_edi = tf->tf_rdi & 0xffffffff;
	regs->r_esi = tf->tf_rsi & 0xffffffff;
	regs->r_ebp = tf->tf_rbp & 0xffffffff;
	regs->r_ebx = tf->tf_rbx & 0xffffffff;
	regs->r_edx = tf->tf_rdx & 0xffffffff;
	regs->r_ecx = tf->tf_rcx & 0xffffffff;
	regs->r_eax = tf->tf_rax & 0xffffffff;
	regs->r_eip = tf->tf_rip & 0xffffffff;
	regs->r_cs = tf->tf_cs;
	regs->r_esp = tf->tf_rsp & 0xffffffff;
	regs->r_ss = tf->tf_ss;

	return (0);
}

int
process_read_fpregs32(struct proc *p, struct fpreg32 *regs)
{
	struct oldfsave frame;

	if (p->p_md.md_flags & MDP_USEDFPU) {
		if (fpuproc == p)
			__asm__("fnsave %0" : "=m" (frame));
	} else {
		memset(&frame, 0, sizeof(*regs));
		frame.fs_control = __NetBSD_NPXCW__;
		frame.fs_tag = 0xffff;
		p->p_md.md_flags |= MDP_USEDFPU;
	}

	memcpy(regs, &frame, sizeof(*regs));
	return (0);
}
