/*	$NetBSD: m68k_syscall.c,v 1.10 2003/07/15 02:43:13 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_syscall.c,v 1.10 2003/07/15 02:43:13 lukem Exp $");

#include "opt_syscall_debug.h"
#include "opt_execfmt.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_aout_m68k.h"

#include <sys/param.h>
#include <sys/systm.h>
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

/*
 * Defined in machine-specific code (usually trap.c)
 * XXX: This will disappear when all m68k ports share common trap() code...
 */
extern void machine_userret(struct lwp *, struct frame *, u_quad_t);

void syscall(register_t, struct frame);

void	syscall_intern(struct proc *);
#ifdef COMPAT_AOUT_M68K
void	aoutm68k_syscall_intern(struct proc *);
#endif
static void syscall_plain(register_t, struct lwp *, struct frame *);
#if defined(KTRACE) || defined(SYSTRACE)
static void syscall_fancy(register_t, struct lwp *, struct frame *);
#endif


/*
 * Process a system call.
 */
void
syscall(code, frame)
	register_t code;
	struct frame frame;
{
	struct lwp *l;
	struct proc *p;
	u_quad_t sticks;

	uvmexp.syscalls++;
	if (!USERMODE(frame.f_sr))
		panic("syscall");

	l = curlwp;
	p = l->l_proc;
	sticks = p->p_sticks;
	l->l_md.md_regs = frame.f_regs;

	(p->p_md.md_syscall)(code, l, &frame);

	machine_userret(l, &frame, sticks);
}

void
syscall_intern(struct proc *p)
{

#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET))
		p->p_md.md_syscall = syscall_fancy;
	else
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE))
		p->p_md.md_syscall = syscall_fancy;
	else
#endif
		p->p_md.md_syscall = syscall_plain;
}

#ifdef COMPAT_AOUT_M68K
/*
 * Not worth the effort of a whole new set of syscall_{plain,fancy} functions
 */
void
aoutm68k_syscall_intern(struct proc *p)
{

#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET))
		p->p_md.md_syscall = syscall_fancy;
	else
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE))
		p->p_md.md_syscall = syscall_fancy;
	else
#endif
		p->p_md.md_syscall = syscall_plain;
}
#endif

static void
syscall_plain(register_t code, struct lwp *l, struct frame *frame)
{
	caddr_t params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];
	struct proc *p = l->l_proc;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (caddr_t)frame->f_regs[SP] + sizeof(int);

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
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;

	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

#ifdef SYSCALL_DEBUG
	scdebug_call(l, code, args);
#endif

	rval[0] = 0;
	rval[1] = frame->f_regs[D1];
	error = (*callp->sy_call)(l, args, rval);

	switch (error) {
	case 0:
		/*
		 * Reinitialize proc pointer `p' as it may be different
		 * if this is a child returning from fork syscall.
		 */
		l = curlwp;
		p = l->l_proc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
#ifdef COMPAT_AOUT_M68K
		{
			extern struct emul emul_netbsd_aoutm68k;

			/*
			 * Some pre-m68k ELF libc assembler stubs assume
			 * %a0 is preserved across system calls...
			 */
			if (p->p_emul != &emul_netbsd_aoutm68k)
				frame->f_regs[A0] = rval[0];
		}
#endif
		break;
	case ERESTART:
		/*
		 * We always enter through a `trap' instruction, which is 2
		 * bytes, so adjust the pc by that amount.
		 */
		frame->f_pc = frame->f_pc - 2;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		/*
		 * XXX: HPUX and SVR4 use this code-path, so we may have
		 * to translate errno.
		 */
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}

#ifdef SYSCALL_DEBUG
        scdebug_ret(p, code, error, rval)
#endif
}

#if defined(KTRACE) || defined(SYSTRACE)
static void
syscall_fancy(register_t code, struct lwp *l, struct frame *frame)
{
	caddr_t params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];
	struct proc *p = l->l_proc;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (caddr_t)frame->f_regs[SP] + sizeof(int);

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
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;

	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	if ((error = trace_enter(l, code, code, NULL, args, rval)) != 0)
		goto bad;

	rval[0] = 0;
	rval[1] = frame->f_regs[D1];
	error = (*callp->sy_call)(l, args, rval);

	switch (error) {
	case 0:
		/*
		 * Reinitialize lwp/proc pointers as they may be different
		 * if this is a child returning from fork syscall.
		 */
		l = curlwp;
		p = l->l_proc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
#ifdef COMPAT_AOUT_M68K
		{
			extern struct emul emul_netbsd_aoutm68k;

			/*
			 * Some pre-m68k ELF libc assembler stubs assume
			 * %a0 is preserved across system calls...
			 */
			if (p->p_emul != &emul_netbsd_aoutm68k)
				frame->f_regs[A0] = rval[0];
		}
#endif
		break;
	case ERESTART:
		/*
		 * We always enter through a `trap' instruction, which is 2
		 * bytes, so adjust the pc by that amount.
		 */
		frame->f_pc = frame->f_pc - 2;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		/*
		 * XXX: HPUX and SVR4 use this code-path, so we may have
		 * to translate errno.
		 */
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(l, code, args, rval, error);
}
#endif /* KTRACE || SYSTRACE */

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
