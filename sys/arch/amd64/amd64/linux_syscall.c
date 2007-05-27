/*	$NetBSD: linux_syscall.c,v 1.13.2.1 2007/05/27 12:26:57 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux_syscall.c,v 1.13.2.1 2007/05/27 12:26:57 ad Exp $");

#include "opt_compat_linux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/arch/amd64/linux_siginfo.h>
#include <compat/linux/arch/amd64/linux_syscall.h>
#include <compat/linux/arch/amd64/linux_machdep.h>

void linux_syscall_intern(struct proc *);
static void linux_syscall_plain(struct trapframe *);
static void linux_syscall_fancy(struct trapframe *);

void
linux_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = linux_syscall_fancy;
	else
		p->p_md.md_syscall = linux_syscall_plain;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
static void
linux_syscall_plain(struct trapframe *frame)
{
	void *params;
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	size_t argsize, argoff;
	register_t code, args[9], rval[2], *argp;

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_rax;
	callp = p->p_emul->e_sysent;
	argoff = 0;
	argp = &args[0];

	code &= (LINUX_SYS_NSYSENT - 1);
	callp += code;

	argsize = (callp->sy_argsize >> 3) + argoff;
	if (argsize) {
		switch (MIN(argsize, 6)) {
		case 6:
			args[5] = frame->tf_r9;
		case 5:
			args[4] = frame->tf_r8;
		case 4:
			args[3] = frame->tf_r10;
		case 3:
			args[2] = frame->tf_rdx;
		case 2:	
			args[1] = frame->tf_rsi;
		case 1:
			args[0] = frame->tf_rdi;
			break;
		default:
			panic("impossible syscall argsize");
		}
		if (argsize > 6) {
			argsize -= 6;
			params = (char *)frame->tf_rsp + sizeof(register_t);
			error = copyin(params, (void *)&args[6],
					argsize << 3);
			if (error != 0)
				goto bad;
		}
	}

	rval[0] = 0;
	rval[1] = 0;
	KERNEL_LOCK(1, l);
	error = (*callp->sy_call)(l, argp, rval);
	KERNEL_UNLOCK_LAST(l);

	switch (error) {
	case 0:
		frame->tf_rax = rval[0];
		frame->tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_rip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		error = native_to_linux_errno[error];
		frame->tf_rax = error;
		frame->tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

	userret(l);
}

static void
linux_syscall_fancy(struct trapframe *frame)
{
	void *params;
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	size_t argsize, argoff;
	register_t code, args[9], rval[2], *argp;

	uvmexp.syscalls++;
	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = frame->tf_rax;
	callp = p->p_emul->e_sysent;
	argp = &args[0];
	argoff = 0;

	code &= (SYS_NSYSENT - 1);
	callp += code;

	argsize = (callp->sy_argsize >> 3) + argoff;
	if (argsize) {
		switch (MIN(argsize, 6)) {
		case 6:
			args[5] = frame->tf_r9;
		case 5:
			args[4] = frame->tf_r8;
		case 4:
			args[3] = frame->tf_r10;
		case 3:
			args[2] = frame->tf_rdx;
		case 2:	
			args[1] = frame->tf_rsi;
		case 1:
			args[0] = frame->tf_rdi;
			break;
		default:
			panic("impossible syscall argsize");
		}
		if (argsize > 6) {
			argsize -= 6;
			params = (char *)frame->tf_rsp + sizeof(register_t);
			error = copyin(params, (void *)&args[6],
					argsize << 3);
			if (error != 0)
				goto bad;
		}
	}

	KERNEL_LOCK(1, l);
	if ((error = trace_enter(l, code, code, NULL, argp)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(l, argp, rval);
out:
	KERNEL_UNLOCK_LAST(l);
	switch (error) {
	case 0:
		frame->tf_rax = rval[0];
		frame->tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame->tf_rip -= frame->tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		error = native_to_linux_errno[error];
		frame->tf_rax = error;
		frame->tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(l, code, argp, rval, error);

	userret(l);
}
