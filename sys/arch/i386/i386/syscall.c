/*	$NetBSD: syscall.c,v 1.28 2004/04/20 12:00:02 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.28 2004/04/20 12:00:02 yamt Exp $");

#include "opt_syscall_debug.h"
#include "opt_vm86.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/savar.h>
#include <sys/user.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#include <sys/syscall.h>


#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

void syscall_intern(struct proc *);
void syscall_plain(struct trapframe *);
void syscall_fancy(struct trapframe *);
#ifdef VM86
void syscall_vm86(struct trapframe *);
#endif

void
syscall_intern(p)
	struct proc *p;
{
#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET)) {
		p->p_md.md_syscall = syscall_fancy;
		return;
	}
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE)) {
		p->p_md.md_syscall = syscall_fancy;
		return;
	} 
#endif
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
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

#ifdef SYSCALL_DEBUG
	scdebug_call(l, code, args);
#endif /* SYSCALL_DEBUG */

	rval[0] = 0;
	rval[1] = 0;

	KASSERT(l->l_holdcnt == 0);

	if (callp->sy_flags & SYCALL_MPSAFE) {
		error = (*callp->sy_call)(l, args, rval);
	} else {
		KERNEL_PROC_LOCK(l);
		error = (*callp->sy_call)(l, args, rval);
		KERNEL_PROC_UNLOCK(l);
	}

	KASSERT(l->l_holdcnt == 0);

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

#ifdef SYSCALL_DEBUG
	scdebug_ret(l, code, error, rval);
#endif /* SYSCALL_DEBUG */
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
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	KERNEL_PROC_LOCK(l);
	if ((error = trace_enter(l, code, code, NULL, args)) != 0) {
		KERNEL_PROC_UNLOCK(l);
		goto bad;
	}

	rval[0] = 0;
	rval[1] = 0;

	KASSERT(l->l_holdcnt == 0);

	if (callp->sy_flags & SYCALL_MPSAFE) {
		KERNEL_PROC_UNLOCK(l);
		error = (*callp->sy_call)(l, args, rval);
	} else {
		error = (*callp->sy_call)(l, args, rval);
		KERNEL_PROC_UNLOCK(l);
	}

	KASSERT(l->l_holdcnt == 0);

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
	KERNEL_PROC_LOCK(l);
	(*p->p_emul->e_trapsignal)(l, &ksi);
	KERNEL_PROC_UNLOCK(l);
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

	KERNEL_PROC_UNLOCK(l);

	userret(l);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(p, SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
}
