/*	$NetBSD: linux_syscall.c,v 1.1.2.4 2001/01/05 17:34:30 bouyer Exp $	*/

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

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_syscall_debug.h"
#include "opt_vm86.h"
#include "opt_ktrace.h"
#endif

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

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/arch/i386/linux_machdep.h>

void linux_syscall_plain __P((struct trapframe));
void linux_syscall_fancy __P((struct trapframe));
extern struct sysent linux_sysent[];

void
linux_syscall_intern(p)
	struct proc *p;
{

#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET))
		p->p_md.md_syscall = linux_syscall_fancy;
	else
#endif
		p->p_md.md_syscall = linux_syscall_plain;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
linux_syscall_plain(frame)
	struct trapframe frame;
{
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_eax;
	callp = linux_sysent;

	code &= (LINUX_SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		/*
		 * Linux passes the args in ebx, ecx, edx, esi, edi, in
		 * increasing order.
		 */
		switch (argsize >> 2) {
		case 5:
			args[4] = frame.tf_edi;
		case 4:
			args[3] = frame.tf_esi;
		case 3:
			args[2] = frame.tf_edx;
		case 2:
			args[1] = frame.tf_ecx;
		case 1:
			args[0] = frame.tf_ebx;
			break;
		default:
			panic("linux syscall bogus argument size %d",
		    		argsize);
			break;
		}
	}
#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif /* SYSCALL_DEBUG */
	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(p, args, rval);
	switch (error) {
	case 0:
		frame.tf_eax = rval[0];
		frame.tf_eflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame.tf_eip -= frame.tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
		error = native_to_linux_errno[error];
		frame.tf_eax = error;
		frame.tf_eflags |= PSL_C;	/* carry bit */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval);
#endif /* SYSCALL_DEBUG */
	userret(p);
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
linux_syscall_fancy(frame)
	struct trapframe frame;
{
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize;
	register_t code, args[8], rval[2];

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_eax;
	callp = linux_sysent;

	code &= (LINUX_SYS_NSYSENT - 1);
	callp += code;
	argsize = callp->sy_argsize;
	if (argsize) {
		/*
		 * Linux passes the args in ebx, ecx, edx, esi, edi, in
		 * increasing order.
		 */
		switch (argsize >> 2) {
		case 5:
			args[4] = frame.tf_edi;
		case 4:
			args[3] = frame.tf_esi;
		case 3:
			args[2] = frame.tf_edx;
		case 2:
			args[1] = frame.tf_ecx;
		case 1:
			args[0] = frame.tf_ebx;
			break;
		default:
			panic("linux syscall bogus argument size %d",
		    		argsize);
			break;
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
	error = (*callp->sy_call)(p, args, rval);
	switch (error) {
	case 0:
		frame.tf_eax = rval[0];
		frame.tf_eflags &= ~PSL_C;	/* carry bit */
		break;
	case ERESTART:
		/*
		 * The offset to adjust the PC by depends on whether we entered
		 * the kernel through the trap or call gate.  We pushed the
		 * size of the instruction into tf_err on entry.
		 */
		frame.tf_eip -= frame.tf_err;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
		error = native_to_linux_errno[error];
		frame.tf_eax = error;
		frame.tf_eflags |= PSL_C;	/* carry bit */
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
