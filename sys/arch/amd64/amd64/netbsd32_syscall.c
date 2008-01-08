/*	$NetBSD: netbsd32_syscall.c,v 1.21.6.2 2008/01/08 22:09:14 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_syscall.c,v 1.21.6.2 2008/01/08 22:09:14 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
/* XXX this file ought to include the netbsd32 version of these 2 headers */
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscall_stats.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

void netbsd32_syscall_intern(struct proc *);
static void netbsd32_syscall(struct trapframe *);

void
netbsd32_syscall_intern(struct proc *p)
{

	p->p_trace_enabled = trace_is_enabled(p);
	p->p_md.md_syscall = netbsd32_syscall;
}

void
netbsd32_syscall(struct trapframe *frame)
{
	char *params;
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	int i;
	register32_t code, args[2 + SYS_MAXSYSARGS];
	register_t rval[2];
	register_t args64[SYS_MAXSYSARGS];

	l = curlwp;
	p = l->l_proc;

	code = frame->tf_rax & (SYS_NSYSENT - 1);
	callp = p->p_emul->e_sysent + code;

	uvmexp.syscalls++;
	LWP_CACHE_CREDS(l, p);

	SYSCALL_COUNT(syscall_counts, code);
	SYSCALL_TIME_SYS_ENTRY(l, syscall_times, code);

	params = (char *)frame->tf_rsp + sizeof(int);

	if (callp->sy_argsize) {
		error = copyin(params, args, callp->sy_argsize);
		if (__predict_false(error != 0))
			goto bad;
		/* Recover 'code' - not in a register */
		code = frame->tf_rax & (SYS_NSYSENT - 1);
	}

	if (__predict_false(p->p_trace_enabled)
	    && !__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
		int narg = callp->sy_argsize >> 2;
		for (i = 0; i < narg; i++)
			args64[i] = args[i];
		error = trace_enter(code, code, NULL, args64);
		if (__predict_false(error != 0))
			goto out;
	}

	rval[0] = 0;
	rval[1] = 0;
	if (callp->sy_flags & SYCALL_MPSAFE)
		error = (*callp->sy_call)(l, args, rval);
	else {
		KERNEL_LOCK(1, l);
		error = (*callp->sy_call)(l, args, rval);
		KERNEL_UNLOCK_LAST(l);
	}

out:
	if (__predict_false(p->p_trace_enabled)
	    && !__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
		/* Recover 'code' - the compiler doesn't assign it a register */
		code = frame->tf_rax & (SYS_NSYSENT - 1);
		trace_exit(code, args64, rval, error);
	}

	if (__predict_true(error == 0)) {
		frame->tf_rax = rval[0];
		frame->tf_rdx = rval[1];
		frame->tf_rflags &= ~PSL_C;	/* carry bit */
	} else {
		switch (error) {
		case ERESTART:
			/*
			 * The offset to adjust the PC by depends on whether we
			 * entered the kernel through the trap or call gate.
			 * We saved the instruction size in tf_err on entry.
			 */
			frame->tf_rip -= frame->tf_err;
			break;
		case EJUSTRETURN:
			/* nothing to do */
			break;
		default:
		bad:
			frame->tf_rax = error;
			frame->tf_rflags |= PSL_C;	/* carry bit */
			break;
		}
	}

	SYSCALL_TIME_SYS_EXIT(l);
	userret(l);
}
