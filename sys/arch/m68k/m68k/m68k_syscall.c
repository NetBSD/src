/*	$NetBSD: m68k_syscall.c,v 1.1.10.5 2002/06/24 22:05:23 nathanw Exp $	*/

/*-
 * Portions Copyright (c) 2000 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: trap.c 1.37 92/12/20$
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/4/94
 */

#include "opt_syscall_debug.h"
#include "opt_execfmt.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_aout_m68k.h"
#include "opt_compat_sunos.h"
#include "opt_compat_hpux.h"
#include "opt_compat_linux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/user.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/reg.h>

#include <uvm/uvm_extern.h>

#ifdef COMPAT_HPUX
#include <compat/hpux/hpux.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_syscall.h>
#include <compat/sunos/sunos_exec.h>
#endif

#ifdef COMPAT_LINUX
extern struct emul emul_linux;
#endif

#ifdef COMPAT_AOUT_M68K
extern struct emul emul_netbsd_aoutm68k;
#endif

/*
 * Defined in machine-specific code (usually trap.c)
 * XXX: This will disappear when all m68k ports share common trap() code...
 */
extern void machine_userret(struct lwp *, struct frame *, u_quad_t);

void syscall(register_t, struct frame);

/*
 * Process a system call.
 */
void
syscall(code, frame)
	register_t code;
	struct frame frame;
{
	caddr_t params;
	const struct sysent *callp;
	struct lwp *l;
	struct proc *p;
	int error, opc, nsys;
	size_t argsize;
	register_t args[8], rval[2];
	u_quad_t sticks;

	uvmexp.syscalls++;
	if (!USERMODE(frame.f_sr))
		panic("syscall");
	l = curlwp;
	p = l->l_proc;
	sticks = p->p_sticks;
	l->l_md.md_regs = frame.f_regs;
	opc = frame.f_pc;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

#ifdef COMPAT_SUNOS
	if (p->p_emul == &emul_sunos) {
		/*
		 * SunOS passes the syscall-number on the stack, whereas
		 * BSD passes it in D0. So, we have to get the real "code"
		 * from the stack, and clean up the stack, as SunOS glue
		 * code assumes the kernel pops the syscall argument the
		 * glue pushed on the stack. Sigh...
		 */
		code = fuword((caddr_t)frame.f_regs[SP]);

		/*
		 * XXX
		 * Don't do this for sunos_sigreturn, as there's no stored pc
		 * on the stack to skip, the argument follows the syscall
		 * number without a gap.
		 */
		if (code != SUNOS_SYS_sigreturn) {
			frame.f_regs[SP] += sizeof (int);
			/*
			 * remember that we adjusted the SP, 
			 * might have to undo this if the system call
			 * returns ERESTART.
			 */
			l->l_md.md_flags |= MDL_STACKADJ;
		} else
			l->l_md.md_flags &= ~MDL_STACKADJ;
	}
#endif

	params = (caddr_t)frame.f_regs[SP] + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		/*
		 * XXX sigreturn requires special stack manipulation
		 * that is only done if entered via the sigreturn
		 * trap.  Cannot allow it here so make sure we fail.
		 */
		switch (code) {
#ifdef COMPAT_13
		case SYS_compat_13_sigreturn13:
#endif
		case SYS___sigreturn14:
			code = nsys;
			break;
		}
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		if (p->p_emul->e_flags & EMUL_HAS_SYS___syscall) {
			code = fuword(params + _QUAD_LOWWORD * sizeof(int));
			params += sizeof(quad_t);
		}
		break;
	default:
		break;
	}
	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;
	argsize = callp->sy_argsize;
#ifdef COMPAT_LINUX
	if (p->p_emul == &emul_linux) {
		/*
		 * Linux passes the args in d1-d5
		 */
		switch (argsize) {
		case 20:
			args[4] = frame.f_regs[D5];
		case 16:
			args[3] = frame.f_regs[D4];
		case 12:
			args[2] = frame.f_regs[D3];
		case 8:
			args[1] = frame.f_regs[D2];
		case 4:
			args[0] = frame.f_regs[D1];
		case 0:
			error = 0;
			break;
		default:
#ifdef DEBUG
			panic("linux syscall %d weird argsize %d",
				code, argsize);
#else
			error = EINVAL;
#endif
			break;
		}
	} else
#endif
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	if ((error = trace_enter(l, code, args, rval)) != 0)
		goto bad;

	rval[0] = 0;
	rval[1] = frame.f_regs[D1];
	error = (*callp->sy_call)(l, args, rval);
	switch (error) {
	case 0:
		/*
		 * Reinitialize lwp/proc pointers as they may be different
		 * if this is a child returning from fork syscall.
		 */
		l = curlwp;
		p = l->l_proc;
		frame.f_regs[D0] = rval[0];
		frame.f_regs[D1] = rval[1];
#ifdef COMPAT_AOUT_M68K
		/*
		 * Some pre-m68k ELF libc assembler stubs assume
		 * %a0 is preserved across system calls...
		 */
		if (p->p_emul != &emul_netbsd_aoutm68k)
			frame.f_regs[A0] = rval[0];
#endif
		frame.f_sr &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * We always enter through a `trap' instruction, which is 2
		 * bytes, so adjust the pc by that amount.
		 */
		frame.f_pc = opc - 2;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame.f_regs[D0] = error;
		frame.f_sr |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(l, code, args, rval, error);

#ifdef COMPAT_SUNOS
	/* need new p-value for this */
	if (l->l_md.md_flags & MDL_STACKADJ) {
		l->l_md.md_flags &= ~MDL_STACKADJ;
		if (error == ERESTART)
			frame.f_regs[SP] -= sizeof (int);
	}
#endif
	machine_userret(l, &frame, sticks);
}

void
child_return(arg)
	void *arg;
{
	struct lwp *l = arg;
	/* See cpu_fork() */
	struct frame *f = (struct frame *)l->l_md.md_regs;

	f->f_regs[D0] = 0;
	f->f_sr &= ~PSL_C;
	f->f_format = FMT0;

	machine_userret(l, f, 0);
#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_SYSRET))
		ktrsysret(l->l_proc, SYS_fork, 0, 0);
#endif
}

/* 
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;
	struct frame *f = (struct frame *)l->l_md.md_regs;

	f->f_regs[D0] = 0;
	f->f_sr &= ~PSL_C;
	f->f_format = FMT0;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	machine_userret(l, f, 0);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(l)
	struct lwp *l;
{
	struct frame *f = (struct frame *)l->l_md.md_regs;

	machine_userret(l, f, 0);
}
