/*	$NetBSD: svr4_machdep.c,v 1.8 2002/07/04 23:32:05 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/vmparam.h>

extern short exframesize[];
extern void	m68881_restore __P((struct fpframe *));
extern void	m68881_save __P((struct fpframe *));
static void	svr4_getsiginfo __P((union svr4_siginfo *, int, unsigned long,
		    caddr_t));

void
svr4_setregs(p, epp, stack)
	struct proc *p;
	struct exec_package *epp;
	unsigned long stack;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	setregs(p, epp, stack);
	frame->f_regs[FP] = (int)stack;
}

void *
svr4_getmcontext(p, mc, flags)
	struct proc *p;
	svr4_mcontext_t *mc;
	unsigned long *flags;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;
	unsigned int format = frame->f_format;
	svr4_greg_t *r = mc->gregs;

	/* Save general register context. */
	r[SVR4_M68K_D0] = frame->f_regs[D0];
	r[SVR4_M68K_D1] = frame->f_regs[D1];
	r[SVR4_M68K_D2] = frame->f_regs[D2];
	r[SVR4_M68K_D3] = frame->f_regs[D3];
	r[SVR4_M68K_D4] = frame->f_regs[D4];
	r[SVR4_M68K_D5] = frame->f_regs[D5];
	r[SVR4_M68K_D6] = frame->f_regs[D6];
	r[SVR4_M68K_D7] = frame->f_regs[D7];
	r[SVR4_M68K_A0] = frame->f_regs[A0];
	r[SVR4_M68K_A1] = frame->f_regs[A1];
	r[SVR4_M68K_A2] = frame->f_regs[A2];
	r[SVR4_M68K_A3] = frame->f_regs[A3];
	r[SVR4_M68K_A4] = frame->f_regs[A4];
	r[SVR4_M68K_A5] = frame->f_regs[A5];
	r[SVR4_M68K_A6] = frame->f_regs[A6];
	r[SVR4_M68K_A7] = frame->f_regs[A7];
	r[SVR4_M68K_PS] = frame->f_sr;
	r[SVR4_M68K_PC] = frame->f_pc;

	/* Save exception frame information. */
	mc->mc_pad.frame.format = format;
	if (format >= FMT4) {
		mc->mc_pad.frame.vector = frame->f_vector;
		(void)memcpy(&mc->mc_pad.frame.exframe, &frame->F_u,
		    (size_t)exframesize[format]);
		
		frame->f_stackadj += exframesize[format];
		frame->f_format = frame->f_vector = 0;
	}
	*flags |= SVR4_UC_CPU;

	if (fputype != 0) {
		/* Save FPU context. */
		struct fpframe fpf;

		m68881_save(&fpf);
		/* Only need to save if not a null frame. */
		if (fpf.fpf_version != 0) {
			mc->fpregs.f_pcr    = fpf.fpf_fpcr;
			mc->fpregs.f_psr    = fpf.fpf_fpsr;
			mc->fpregs.f_piaddr = fpf.fpf_fpiar;
			(void)memcpy(&mc->fpregs.f_fpregs, &fpf.fpf_regs,
			    sizeof (mc->fpregs.f_fpregs));
			*flags |= SVR4_UC_FPU;
		}
	}

	return (void *)frame->f_regs[A7];	/* Nonsense */
}

int
svr4_setmcontext(p, mc, flags)
	struct proc *p;
	svr4_mcontext_t *mc;
	unsigned long flags;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;
	unsigned int format = mc->mc_pad.frame.format;
	svr4_greg_t *r = mc->gregs;
	int sz;
	
	if ((flags & SVR4_UC_CPU) != 0) {	
		/* Validate general register context. */
		if ((r[SVR4_M68K_PS] & (PSL_MBZ|PSL_IPL|PSL_S)) != 0 ||
		    format > 0xf || (sz = exframesize[format]) < 0) {
			return (EINVAL);
		}
		
		/* Restore exception frame information. */
		if (format >= FMT4) {
			if (frame->f_stackadj == 0) {
				reenter_syscall(frame, sz);
				/* NOTREACHED */
			}
#ifdef DIAGNOSTIC
			if (sz != frame->f_stackadj)
				panic("svr4_setmcontext: %d != %d",
				    sz, frame->f_stackadj);
#endif

			frame->f_format = format;
			frame->f_vector = mc->mc_pad.frame.vector;
			(void)memcpy(&frame->F_u,
			    &mc->mc_pad.frame.exframe,
			    (size_t)sz);
			frame->f_stackadj -= sz;
		}

		/* Restore general register context. */
		frame->f_regs[D0] = r[SVR4_M68K_D0];
		frame->f_regs[D1] = r[SVR4_M68K_D1];
		frame->f_regs[D2] = r[SVR4_M68K_D2];
		frame->f_regs[D3] = r[SVR4_M68K_D3];
		frame->f_regs[D4] = r[SVR4_M68K_D4];
		frame->f_regs[D5] = r[SVR4_M68K_D5];
		frame->f_regs[D6] = r[SVR4_M68K_D6];
		frame->f_regs[D7] = r[SVR4_M68K_D7];
		frame->f_regs[A0] = r[SVR4_M68K_A0];
		frame->f_regs[A1] = r[SVR4_M68K_A1];
		frame->f_regs[A2] = r[SVR4_M68K_A2];
		frame->f_regs[A3] = r[SVR4_M68K_A3];
		frame->f_regs[A4] = r[SVR4_M68K_A4];
		frame->f_regs[A5] = r[SVR4_M68K_A5];
		frame->f_regs[A6] = r[SVR4_M68K_A6];
		frame->f_regs[A7] = r[SVR4_M68K_A7];
		frame->f_sr = r[SVR4_M68K_PS];
		frame->f_pc = r[SVR4_M68K_PC];

	}

	if ((flags & SVR4_UC_FPU) != 0 && fputype != 0) {
		/* Restore FPU context. */
		struct fpframe fpf;

		fpf.fpf_fpcr  = mc->fpregs.f_pcr;
		fpf.fpf_fpsr  = mc->fpregs.f_psr;
		fpf.fpf_fpiar = mc->fpregs.f_piaddr;
		(void)memcpy(&fpf.fpf_regs, &mc->fpregs.f_fpregs,
		    sizeof (fpf.fpf_regs));

		m68881_restore(&fpf);
	}

	return (0);
}

static void
svr4_getsiginfo(sip, sig, code, addr)
	union svr4_siginfo *	sip;
	int			sig;
	unsigned long		code;
	caddr_t			addr;
{

	/*
	 * The m68k implementation(s) of trap() are braindamaged as they
	 * pass either exception codes, faulting memory addresses or even
	 * frame format codes in the trapsignal() `code' argument, which
	 * makes extracting accurate/useful information impossible, or
	 * at least quite heard.
	 *
	 * The Answer: a native `siginfo' implementation, soon to appear
	 * at a place near you.
	 */

	sip->si_signo = native_to_svr4_signo[sig];
	sip->si_errno = 0;
	sip->si_code  = 0;	/* reserved, `no information' */
	sip->si_addr  = addr;	/* XXX not necessarily correct */
}

void
svr4_sendsig(sig, mask, code)
	int sig;
	sigset_t *mask;
	unsigned long code;
{
	struct proc *p = curproc;
	struct frame *frame;
	struct svr4_sigframe *sfp, sf;
	int onstack;
	sig_t catcher = SIGACTION(p, sig).sa_handler;

	frame = (struct frame *)p->p_md.md_regs;

	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		sfp = (struct svr4_sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
		    p->p_sigctx.ps_sigstk.ss_size);
	else
		sfp = (struct svr4_sigframe *)frame->f_regs[SP];
	sfp--;

	svr4_getcontext(p, &sf.sf_uc, mask);
	/* Passing the PC is *wrong*! */
	svr4_getsiginfo(&sf.sf_si, sig, code, (caddr_t)frame->f_pc);

	/* Build stack frame for signal trampoline. */
	sf.sf_signum = sf.sf_si.si_signo;
	sf.sf_sip = &sfp->sf_si;
	sf.sf_ucp = &sfp->sf_uc;
	sf.sf_handler = catcher;

#ifdef DEBUG_SVR4
	printf("sig = %d, sip %p, ucp = %p, handler = %p\n",
	    sf.sf_signum, sf.sf_sip, sf.sf_ucp, sf.sf_handler);
#endif  

	if(copyout(&sf, sfp, sizeof (sf)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/* Set up the registers to return to sigcode. */
	frame->f_regs[SP] = (int)sfp;
	frame->f_pc = (int)p->p_sigctx.ps_sigcode;

	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
}

/*
 * sysm68k()
 */
int
svr4_sys_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(void *) a1;
	} */ *uap = v;
	char tmp[MAXHOSTNAMELEN];
	size_t len;
	int error, name;

	switch (SCARG(uap, op)) {
	case SVR4_SYSARCH_SETNAME:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		if ((error = copyinstr(SCARG(uap, a1), tmp, sizeof (tmp), &len))
		    != 0)
			return (error);
		name = KERN_HOSTNAME;
		return (kern_sysctl(&name, 1, NULL, NULL, SCARG(uap, a1), len,
		    p));
	default:
		printf("uninplemented svr4_sysarch(%d), a1 %p\n",
		    SCARG(uap, op), SCARG(uap, a1));
		error = EINVAL;
	}

	return (error);
}
