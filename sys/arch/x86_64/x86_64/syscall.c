/*	$NetBSD: syscall.c,v 1.3 2002/06/03 18:23:17 fvdl Exp $	*/

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

#include "opt_syscall_debug.h"
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

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

void syscall_intern __P((struct proc *));
void syscall_plain __P((struct trapframe));
void syscall_fancy __P((struct trapframe));

void
syscall_intern(p)
	struct proc *p;
{

#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET))
		p->p_md.md_syscall = syscall_fancy;
	else
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
	struct trapframe frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize, argoff;
	register_t code, args[9], rval[2], *argp;

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_rax;
	callp = p->p_emul->e_sysent;
	argoff = 0;
	argp = &args[0];

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = frame.tf_rdi;
		argp = &args[1];
		argoff = 1;
		break;
	default:
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;

	argsize = (callp->sy_argsize >> 3) + argoff;
	if (argsize) {
		switch (MIN(argsize, 6)) {
		case 6:
			args[5] = frame.tf_r9;
		case 5:
			args[4] = frame.tf_r8;
		case 4:
			args[3] = frame.tf_r10;
		case 3:
			args[2] = frame.tf_rdx;
		case 2:	
			args[1] = frame.tf_rsi;
		case 1:
			args[0] = frame.tf_rdi;
			break;
		default:
			panic("impossible syscall argsize");
		}
		if (argsize > 6) {
			argsize -= 6;
			params = (caddr_t)frame.tf_rsp + sizeof(register_t);
			error = copyin(params, (caddr_t)&args[6],
					argsize << 3);
			if (error != 0)
				goto bad;
		}
	}

#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, argp);
#endif /* SYSCALL_DEBUG */

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(p, argp, rval);
	switch (error) {
	case 0:
		frame.tf_rax = rval[0];
		frame.tf_rdx = rval[1];
		frame.tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame.tf_rip -= frame.tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		frame.tf_rax = error;
		frame.tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval);
#endif /* SYSCALL_DEBUG */
	userret(p);
}

void
syscall_fancy(frame)
	struct trapframe frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize, argoff;
	register_t code, args[9], rval[2], *argp;

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_rax;
	callp = p->p_emul->e_sysent;
	argp = &args[0];
	argoff = 0;

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = frame.tf_rdi;
		argp = &args[1];
		argoff = 1;
		break;
	default:
		break;
	}
	code &= (SYS_NSYSENT - 1);
	callp += code;

	argsize = (callp->sy_argsize >> 3) + argoff;
	if (argsize) {
		switch (MIN(argsize, 6)) {
		case 6:
			args[5] = frame.tf_r9;
		case 5:
			args[4] = frame.tf_r8;
		case 4:
			args[3] = frame.tf_r10;
		case 3:
			args[2] = frame.tf_rdx;
		case 2:	
			args[1] = frame.tf_rsi;
		case 1:
			args[0] = frame.tf_rdi;
			break;
		default:
			panic("impossible syscall argsize");
		}
		if (argsize > 6) {
			argsize -= 6;
			params = (caddr_t)frame.tf_rsp + sizeof(register_t);
			error = copyin(params, (caddr_t)&args[6],
					argsize << 3);
			if (error != 0)
				goto bad;
		}
	}


#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif /* SYSCALL_DEBUG */
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, argsize, args);
#endif /* KTRACE */

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(p, argp, rval);
	switch (error) {
	case 0:
		frame.tf_rax = rval[0];
		frame.tf_rdx = rval[1];
		frame.tf_rflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame.tf_rip -= frame.tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		frame.tf_rax = error;
		frame.tf_rflags |= PSL_C;	/* carry bit */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval);
#endif /* SYSCALL_DEBUG */
	userret(p);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif /* KTRACE */
}

void
child_return(arg)
	void *arg;
{
	struct proc *p = arg;
	struct trapframe *tf = p->p_md.md_regs;

	tf->tf_rax = 0;
	tf->tf_rflags &= ~PSL_C;

	userret(p);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}
