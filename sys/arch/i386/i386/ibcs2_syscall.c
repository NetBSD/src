/*	$NetBSD: ibcs2_syscall.c,v 1.28.6.1 2006/04/22 11:37:32 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_syscall.c,v 1.28.6.1 2006/04/22 11:37:32 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/savar.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

#include <compat/ibcs2/ibcs2_errno.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_syscall.h>
#include <machine/ibcs2_machdep.h>

void ibcs2_syscall_plain(struct trapframe *);
void ibcs2_syscall_fancy(struct trapframe *);
extern struct sysent ibcs2_sysent[];

void
ibcs2_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = ibcs2_syscall_fancy;
	else
		p->p_md.md_syscall = ibcs2_syscall_plain;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
ibcs2_syscall_plain(frame)
	struct trapframe *frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	struct lwp *l;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	l = curlwp;

	code = frame->tf_eax;
	if (IBCS2_HIGH_SYSCALL(code))
		code = IBCS2_CVT_HIGH_SYSCALL(code);
	callp = ibcs2_sysent;
	params = (caddr_t)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		break;
	default:
		break;
	}

	code &= (IBCS2_SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	rval[0] = 0;
	rval[1] = 0;

	KERNEL_PROC_LOCK(l);
	error = (*callp->sy_call)(l, args, rval);
	KERNEL_PROC_UNLOCK(l);	

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
		error = native_to_ibcs2_errno[error];
		frame->tf_eax = error;
		frame->tf_eflags |= PSL_C;	/* carry bit */
		break;
	}

	userret(l);
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
ibcs2_syscall_fancy(frame)
	struct trapframe *frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	struct lwp *l;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	l = curlwp;

	code = frame->tf_eax;
	if (IBCS2_HIGH_SYSCALL(code))
		code = IBCS2_CVT_HIGH_SYSCALL(code);
	callp = ibcs2_sysent;
	params = (caddr_t)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		break;
	default:
		break;
	}

	code &= (IBCS2_SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (caddr_t)args, argsize);
		if (error)
			goto bad;
	}

	KERNEL_PROC_LOCK(l);
	if ((error = trace_enter(l, code, code, NULL, args)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(l, args, rval);
out:
	KERNEL_PROC_UNLOCK(l);
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
		error = native_to_ibcs2_errno[error];
		frame->tf_eax = error;
		frame->tf_eflags |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(l, code, args, rval, error);

	userret(l);
}
