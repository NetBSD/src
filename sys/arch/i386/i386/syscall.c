/*	$NetBSD: syscall.c,v 1.42.2.1 2007/02/27 16:51:45 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.42.2.1 2007/02/27 16:51:45 yamt Exp $");

#include "opt_vm86.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <sys/syscall.h>
#include <sys/syscall_stats.h>


#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

void syscall_plain(struct trapframe *);
void syscall_fancy(struct trapframe *);
#ifdef VM86
void syscall_vm86(struct trapframe *);
#endif

void
syscall_intern(p)
	struct proc *p;
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
syscall_plain(frame)
	struct trapframe *frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct lwp *l;
	register struct proc *p;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_eax;
	callp = p->p_emul->e_sysent;
	params = (caddr_t)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		SYSCALL_COUNT(syscall_counts, SYS_syscall & (SYS_NSYSENT - 1));
		code = fuword(params);
		params += sizeof(int);
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		SYSCALL_COUNT(syscall_counts, SYS___syscall & (SYS_NSYSENT - 1));
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	code &= (SYS_NSYSENT - 1);
	SYSCALL_COUNT(syscall_counts, code);
	SYSCALL_TIME_SYS_ENTRY(l, syscall_times, code);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	rval[0] = 0;
	rval[1] = 0;

	KASSERT(l->l_holdcnt == 0);

	if (callp->sy_flags & SYCALL_MPSAFE) {
		error = (*callp->sy_call)(l, args, rval);
	} else {
		KERNEL_LOCK(1, l);
		error = (*callp->sy_call)(l, args, rval);
		KERNEL_UNLOCK_LAST(l);
	}

#if defined(DIAGNOSTIC)
	if (l->l_holdcnt != 0)
		panic("l_holdcnt leak (%d) in syscall %d\n",
		    l->l_holdcnt, (int)code);
#endif /* defined(DIAGNOSTIC) */

	switch (error) {
	case 0:
		frame->tf_eax = rval[0];
		frame->tf_edx = rval[1];
		frame->tf_eflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_eip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		frame->tf_eax = error;
		frame->tf_eflags |= PSL_C;	/* carry bit */
		break;
	}

	SYSCALL_TIME_SYS_EXIT(l);
	userret(l);
}

void
syscall_fancy(frame)
	struct trapframe *frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct lwp *l;
	register struct proc *p;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_eax;
	callp = p->p_emul->e_sysent;
	params = (caddr_t)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
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

	code &= (SYS_NSYSENT - 1);
	SYSCALL_COUNT(syscall_counts, code);
	SYSCALL_TIME_SYS_ENTRY(l, syscall_times, code);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	KERNEL_LOCK(1, l);
	if ((error = trace_enter(l, code, code, NULL, args)) != 0) {
		KERNEL_UNLOCK_LAST(l);
		goto out;
	}

	rval[0] = 0;
	rval[1] = 0;

	KASSERT(l->l_holdcnt == 0);

	if (callp->sy_flags & SYCALL_MPSAFE) {
		KERNEL_UNLOCK_LAST(l);
		error = (*callp->sy_call)(l, args, rval);
	} else {
		error = (*callp->sy_call)(l, args, rval);
		KERNEL_UNLOCK_LAST(l);
	}

#if defined(DIAGNOSTIC)
	if (l->l_holdcnt != 0)
		panic("l_holdcnt leak (%d) in syscall %d\n",
		    l->l_holdcnt, (int)code);
#endif /* defined(DIAGNOSTIC) */
out:
	switch (error) {
	case 0:
		frame->tf_eax = rval[0];
		frame->tf_edx = rval[1];
		frame->tf_eflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_eip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		frame->tf_eax = error;
		frame->tf_eflags |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(l, code, args, rval, error);

	SYSCALL_TIME_SYS_EXIT(l);
	userret(l);
}

#ifdef VM86
void
syscall_vm86(frame)
	struct trapframe *frame;
{
	struct lwp *l;
	struct proc *p;
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGBUS;
	ksi.ksi_code = BUS_OBJERR;
	ksi.ksi_trap = T_PROTFLT;
	ksi.ksi_addr = (void *)frame->tf_eip;

	l = curlwp;
	p = l->l_proc;
	KERNEL_LOCK(1, l);
	(*p->p_emul->e_trapsignal)(l, &ksi);
	KERNEL_UNLOCK_LAST(l);
	userret(l);
}
#endif

void
child_return(arg)
	void *arg;
{
	struct lwp *l = arg;
	struct trapframe *tf = l->l_md.md_regs;
#ifdef KTRACE
	struct proc *p = l->l_proc;
#endif

	tf->tf_eax = 0;
	tf->tf_eflags &= ~PSL_C;

	KERNEL_UNLOCK_LAST(l);

	userret(l);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_LOCK(1, l);
		ktrsysret(l, SYS_fork, 0, 0);
		KERNEL_UNLOCK_LAST(l);
	}
#endif
}
