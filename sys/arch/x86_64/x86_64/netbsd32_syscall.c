/*	$NetBSD: netbsd32_syscall.c,v 1.5 2002/06/17 16:33:21 christos Exp $	*/

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
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
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

void netbsd32_syscall_intern(struct proc *);
void netbsd32_syscall_plain(struct trapframe);
void netbsd32_syscall_fancy(struct trapframe);

void
netbsd32_syscall_intern(p)
	struct proc *p;
{
#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET)) {
		p->p_md.md_syscall = netbsd32_syscall_fancy;
		return;
	}
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE)) {
		p->p_md.md_syscall = netbsd32_syscall_fancy;
		return;
	} 
#endif
	p->p_md.md_syscall = netbsd32_syscall_plain;
}

void
netbsd32_syscall_plain(frame)
	struct trapframe frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize;
	register32_t code, args[8];
	register_t rval[2];

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_rax;
	callp = p->p_emul->e_sysent;
	params = (caddr_t)frame.tf_rsp + sizeof(int);

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
	scdebug_call(p, code, args);
#endif /* SYSCALL_DEBUG */

	rval[0] = 0;
	rval[1] = 0;
#if 0
	printf("netbsd32: syscall %d (%x %x %x %x %x %x, %x)\n", code,
	    args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
#endif
	error = (*callp->sy_call)(p, args, rval);
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
netbsd32_syscall_fancy(frame)
	struct trapframe frame;
{
	register caddr_t params;
	register const struct sysent *callp;
	register struct proc *p;
	int error;
	size_t argsize;
	register32_t code, args[8];
	register_t rval[2];
#if defined(KTRACE) || defined(SYSTRACE)
	int i;
	register_t args64[8];
#endif

	uvmexp.syscalls++;
	p = curproc;

	code = frame.tf_rax;
	callp = p->p_emul->e_sysent;
	params = (caddr_t)frame.tf_rsp + sizeof(int);

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

	if (KTRPOINT(p, KTR_SYSCALL) || ISSET(p->p_flag, P_SYSTRACE)) {
		for (i = 0; i < (argsize >> 2); i++)
			args64[i] = args[i];
		/* XXX we need to pass argsize << 1 here? */
		if ((error = trace_enter(p, code, args64, rval)) != 0)
			goto bad;
	}

	rval[0] = 0;
	rval[1] = 0;
	error = (*callp->sy_call)(p, args, rval);
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

	trace_exit(p, code, args64, rval, error);

	userret(p);
}
