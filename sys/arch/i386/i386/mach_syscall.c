/*	$NetBSD: mach_syscall.c,v 1.33.8.2 2009/11/01 13:58:21 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mach_syscall.c,v 1.33.8.2 2009/11/01 13:58:21 jym Exp $");

#include "opt_vm86.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_syscall.h>
#include <compat/mach/mach_syscallargs.h>

void mach_syscall_intern(struct proc *);
void mach_syscall_plain(struct trapframe *);
void mach_syscall_fancy(struct trapframe *);

extern struct sysent mach_sysent[];

void
mach_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = mach_syscall_fancy;
	else
		p->p_md.md_syscall = mach_syscall_plain;
}


/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
mach_syscall_plain(struct trapframe *frame)
{
	char *params;
	const struct sysent *callp;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int error;
	size_t argsize;
	register_t code, args[MACH_SYS_MAXSYSARGS], rval[2];

	LWP_CACHE_CREDS(l, p);

	code = frame->tf_eax;
	params = (char *)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		uprintf("->syscall(%d)\n", code);
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		uprintf("->__syscall(%d)\n", code);
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (code < 0) {
		uprintf("->mach(%d)\n", code);
		code = -code;
		callp = mach_sysent;
		code &= (MACH_SYS_NSYSENT - 1);
	} else {
		callp = p->p_emul->e_sysent;
		code &= (SYS_NSYSENT - 1);
	}

	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (void *)args, argsize);
		if (error)
			goto bad;
	}

	rval[0] = 0;
	rval[1] = 0;
	KERNEL_LOCK(1, NULL);
	error = sy_call(callp, l, args, rval);
	KERNEL_UNLOCK_ONE(NULL);
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

	userret(l);
}

void
mach_syscall_fancy(struct trapframe *frame)
{
	char *params;
	const struct sysent *callp;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	int error;
	size_t argsize;
	register_t code, realcode, args[MACH_SYS_MAXSYSARGS], rval[2];

	LWP_CACHE_CREDS(l, p);

	code = frame->tf_eax;
	realcode = code;
	params = (char *)frame->tf_esp + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
		uprintf("->syscall(%d)\n", code);
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		uprintf("->__syscall(%d)\n", code);
		break;
	default:
		break;
	}

	if (code < 0) {
		uprintf("->mach(%d)\n", code);
		code = -code;
		callp = mach_sysent;
		code &= (MACH_SYS_NSYSENT - 1);
	} else {
		callp = p->p_emul->e_sysent;
		code &= (SYS_NSYSENT - 1);
	}
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (void *)args, argsize);
		if (error)
			goto bad;
	}

	if ((error = trace_enter(realcode, args, callp->sy_narg)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = 0;
	KERNEL_LOCK(1, NULL);
	error = sy_call(callp, l, args, rval);
	KERNEL_UNLOCK_ONE(NULL);
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

	trace_exit(realcode, rval, error);

	userret(l);
}
