/*	$NetBSD: linux_machdep.c,v 1.1 1995/04/07 22:29:34 fvdl Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <compat/linux/linux_types.h>
#include <compat/linux/linux_syscallargs.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/linux_machdep.h>

/*
 * Deal with some i386-specific things in the Linux emulation code.
 * This means just signals for now, will include stuff like
 * I/O map permissions and V86 mode sometime.
 */

extern int _udatasel, _ucodesel;

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
linux_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct trapframe *tf;
	struct linux_sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern char linux_sigcode[], linux_esigcode[];

	tf = (struct trapframe *)p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct linux_sigframe *)(psp->ps_sigstk.ss_base +
		    psp->ps_sigstk.ss_size - sizeof(struct linux_sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else {
		fp = (struct linux_sigframe *)tf->tf_esp - 1;
	}

	frame.ls_handler = catcher;
	frame.ls_sig = bsd_to_linux_sig(sig);

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	frame.ls_sc.lsc_mask   = mask;
	frame.ls_sc.lsc_es     = tf->tf_es;
	frame.ls_sc.lsc_fs     = _udatasel;
	frame.ls_sc.lsc_gs     = _udatasel;
	frame.ls_sc.lsc_ds     = tf->tf_ds;
	frame.ls_sc.lsc_edi    = tf->tf_edi;
	frame.ls_sc.lsc_esi    = tf->tf_esi;
	frame.ls_sc.lsc_ebp    = tf->tf_ebp;
	frame.ls_sc.lsc_ebx    = tf->tf_ebx;
	frame.ls_sc.lsc_edx    = tf->tf_edx;
	frame.ls_sc.lsc_ecx    = tf->tf_ecx;
	frame.ls_sc.lsc_eax    = tf->tf_eax;
	frame.ls_sc.lsc_eip    = tf->tf_eip;
	frame.ls_sc.lsc_cs     = tf->tf_cs;
	frame.ls_sc.lsc_eflags = tf->tf_eflags;
	frame.ls_sc.lsc_esp    = tf->tf_esp;
	frame.ls_sc.lsc_ss     = tf->tf_ss;
	frame.ls_sc.lsc_err    = tf->tf_err;
	frame.ls_sc.lsc_trapno = tf->tf_trapno;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
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
	tf->tf_esp = (int)fp;
	tf->tf_eip = (int)(((char *)PS_STRINGS) -
	     (linux_esigcode - linux_sigcode));
	tf->tf_eflags &= ~PSL_VM;
	tf->tf_cs = _ucodesel;
	tf->tf_ds = _udatasel;
	tf->tf_es = _udatasel;
	tf->tf_ss = _udatasel;
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
linux_sigreturn(p, uap, retval)
	struct proc *p;
	struct linux_sigreturn_args /* {
		syscallarg(struct linux_sigcontext *) scp;
	} */ *uap;
	register_t *retval;
{
	struct linux_sigcontext *scp, context;
	register struct trapframe *tf;

	tf = (struct trapframe *)p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, scp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/*
	 * Check for security violations.
	 */
	if (((context.lsc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
	    ISPL(context.lsc_cs) != SEL_UPL)
		return (EINVAL);

	p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = context.lsc_mask & ~sigcantmask;

	/*
	 * Restore signal context.
	 */
	tf->tf_es     = context.lsc_es;
	tf->tf_ds     = context.lsc_ds;
	tf->tf_edi    = context.lsc_edi;
	tf->tf_esi    = context.lsc_esi;
	tf->tf_ebp    = context.lsc_ebp;
	tf->tf_ebx    = context.lsc_ebx;
	tf->tf_edx    = context.lsc_edx;
	tf->tf_ecx    = context.lsc_ecx;
	tf->tf_eax    = context.lsc_eax;
	tf->tf_eip    = context.lsc_eip;
	tf->tf_cs     = context.lsc_cs;
	tf->tf_eflags = context.lsc_eflags;
	tf->tf_esp    = context.lsc_esp;
	tf->tf_ss     = context.lsc_ss;

	return (EJUSTRETURN);
}
