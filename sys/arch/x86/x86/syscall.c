/*	$NetBSD: syscall.c,v 1.11.2.1 2014/08/20 00:03:29 tls Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2009 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.11.2.1 2014/08/20 00:03:29 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/ktrace.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscall_stats.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/userret.h>

#ifndef __x86_64__
#include "opt_vm86.h"
#ifdef VM86
void		syscall_vm86(struct trapframe *);
#endif
int		x86_copyargs(void *, void *, size_t);
#endif

void		syscall_intern(struct proc *);
static void	syscall(struct trapframe *);

void
child_return(void *arg)
{
	struct lwp *l = arg;
	struct trapframe *tf = l->l_md.md_regs;
	struct proc *p = l->l_proc;

	if (p->p_slflag & PSL_TRACED) {
		ksiginfo_t ksi;

		mutex_enter(proc_lock);
                KSI_INIT_EMPTY(&ksi);
                ksi.ksi_signo = SIGTRAP;
                ksi.ksi_lid = l->l_lid; 
                kpsignal(p, &ksi, NULL);
		mutex_exit(proc_lock);
	}

	X86_TF_RAX(tf) = 0;
	X86_TF_RFLAGS(tf) &= ~PSL_C;

	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

/*
 * Process the tail end of a posix_spawn() for the child.
 */
void
cpu_spawn_return(struct lwp *l)
{

	userret(l);
}
	
void
syscall_intern(struct proc *p)
{

	p->p_md.md_syscall = syscall;
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 *	Like trap(), argument is call by reference.
 */
static void
syscall(struct trapframe *frame)
{
	const struct sysent *callp;
	struct proc *p;
	struct lwp *l;
	int error;
	register_t code, rval[2];
#ifdef __x86_64__
	/* Verify that the syscall args will fit in the trapframe space */
	CTASSERT(offsetof(struct trapframe, tf_arg9) >=
	    sizeof(register_t) * (2 + SYS_MAXSYSARGS - 1));
#define args (&frame->tf_rdi)
#else
	register_t args[2 + SYS_MAXSYSARGS];
#endif

	l = curlwp;
	p = l->l_proc;
	LWP_CACHE_CREDS(l, p);

	code = X86_TF_RAX(frame) & (SYS_NSYSENT - 1);
	callp = p->p_emul->e_sysent + code;

	SYSCALL_COUNT(syscall_counts, code);
	SYSCALL_TIME_SYS_ENTRY(l, syscall_times, code);

#ifdef __x86_64__
	/*
	 * The first 6 syscall args are passed in rdi, rsi, rdx, r10, r8 and r9
	 * (rcx gets copied to r10 in the libc stub because the syscall
	 * instruction overwrites %cx) and are together in the trap frame
	 * with space following for 4 more entries.
	 */
	if (__predict_false(callp->sy_argsize > 6 * 8)) {
		error = copyin((register_t *)frame->tf_rsp + 1,
		    &frame->tf_arg6, callp->sy_argsize - 6 * 8);
		if (error != 0)
			goto bad;
	}
#else
	if (callp->sy_argsize) {
		error = x86_copyargs((char *)frame->tf_esp + sizeof(int), args,
			    callp->sy_argsize);
		if (__predict_false(error != 0))
			goto bad;
	}
#endif
	error = sy_invoke(callp, l, args, rval, code);

	if (__predict_true(error == 0)) {
		X86_TF_RAX(frame) = rval[0];
		X86_TF_RDX(frame) = rval[1];
		X86_TF_RFLAGS(frame) &= ~PSL_C;	/* carry bit */
	} else {
		switch (error) {
		case ERESTART:
			/*
			 * The offset to adjust the PC by depends on whether we
			 * entered the kernel through the trap or call gate.
			 * We saved the instruction size in tf_err on entry.
			 */
			X86_TF_RIP(frame) -= frame->tf_err;
			break;
		case EJUSTRETURN:
			/* nothing to do */
			break;
		default:
		bad:
			X86_TF_RAX(frame) = error;
			X86_TF_RFLAGS(frame) |= PSL_C;	/* carry bit */
			break;
		}
	}

	SYSCALL_TIME_SYS_EXIT(l);
	userret(l);
}

#ifdef VM86

void
syscall_vm86(struct trapframe *frame)
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

	(*p->p_emul->e_trapsignal)(l, &ksi);
	userret(l);
}

#endif
