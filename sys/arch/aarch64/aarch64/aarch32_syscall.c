/*	$NetBSD: aarch32_syscall.c,v 1.1 2018/10/12 01:28:57 ryo Exp $	*/

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aarch32_syscall.c,v 1.1 2018/10/12 01:28:57 ryo Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/syscallvar.h>
#include <uvm/uvm_extern.h>

#include <aarch64/userret.h>
#include <aarch64/frame.h>
#include <aarch64/armreg.h>

#ifndef EMULNAME
#error EMULNAME is not defined
#endif

#ifndef NARGREG
#define NARGREG		4		/* 4 args are in registers */
#endif

static void EMULNAME(syscall)(struct trapframe *);

union args {
	register_t a64[EMULNAMEU(SYS_MAXSYSARGS)];
	register32_t a32[EMULNAMEU(SYS_MAXSYSARGS)];
};

void
EMULNAME(syscall)(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	const struct sysent *callp;
	union args args64buf, args32buf;
	register_t rval[2];
	register32_t *args32 = args32buf.a32;
	int error, i;
	bool do_trace;

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_data.cpu_nsyscall++;

	uint32_t code = tf->tf_esr & 0xffff;	/* XXX: 16-23bits are omitted */

	/*
	 * XXX: for netbsd32 emulation, SWI_OS_NETBSD should be checked?
	 * 16-23bits of imm of swi is omitted. need to read insn?
	 */
#ifdef THUMB_CODE
#error notyet
	if (tf->tf_spsr & SPSR_A32_T) {
		code |= tf->tf_reg[0];
		tf->tf_reg[0] = tf->tf_reg[12];	/* r0 = ip(r12) */
	}
#endif

	int nargs_reg = NARGREG;	/* number of argument in registers */
	int regstart = 0;		/* args start from r0 */

	code %= EMULNAMEU(SYS_NSYSENT);
	callp = p->p_emul->e_sysent + code;
	if (__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
		nargs_reg -= 1;
		regstart = 1;	/* args start from r1 */
		code = tf->tf_reg[0] % EMULNAMEU(SYS_NSYSENT);
		callp = p->p_emul->e_sysent + code;

		/* don't allow nested syscall */
		if (__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
			error = EINVAL;
			goto bad;
		}
	}

	/* number of argument to fetch from sp */
	KASSERT(callp->sy_narg <= EMULNAMEU(SYS_MAXSYSARGS));
	int nargs_sp = callp->sy_narg - nargs_reg;

	/* fetch arguments from tf and sp, and store to args32buf[] */
	for (i = 0; i < nargs_reg; i++)
		*args32++ = (uint32_t)tf->tf_reg[regstart++];
	if (nargs_sp > 0) {
		error = copyin(
		    (void*)(uintptr_t)(uint32_t)tf->tf_reg[13],	/* sp = r13 */
		    args32, nargs_sp * sizeof(register32_t));
		if (error)
			goto bad;
	}

	rval[0] = rval[1] = 0;

#if 0
	error = sy_invoke(callp, l, args32buf.a32, rval, code);
#else
	/*
	 * XXX: trace_enter()/trace_exit() called from sy_invoke() expects
	 *      64bit args, but sy_invoke doesn't take care of it.
	 *      therefore call trace_enter(), sy_call(), trace_exit() manually.
	 */
#ifdef KDTRACE_HOOKS
#define KDTRACE_ENTRY(a)	(a)
#else
#define KDTRACE_ENTRY(a)	(0)
#endif
	do_trace = p->p_trace_enabled &&
	    ((callp->sy_flags & SYCALL_INDIRECT) == 0);
	if (__predict_false(do_trace ||
	    KDTRACE_ENTRY(callp->sy_entry) || KDTRACE_ENTRY(callp->sy_return))) {
		/* build 64bit args for trace_enter()/trace_exit() */
		int nargs = callp->sy_narg;
		for (i = 0; i < nargs; i++)
			args64buf.a64[i] = args32buf.a32[i];
	}

	if (__predict_false(do_trace || KDTRACE_ENTRY(callp->sy_entry)))
		error = trace_enter(code, callp, args64buf.a64);

	if (error == 0)
		error = sy_call(callp, l, args32buf.a32, rval);

	if (__predict_false(do_trace || KDTRACE_ENTRY(callp->sy_return)))
		trace_exit(code, callp, args64buf.a64, rval, error);
#endif

	if (__predict_true(error == 0)) {
		tf->tf_reg[0] = rval[0];
		tf->tf_reg[1] = rval[1];
		tf->tf_spsr &= ~NZCV_C;
	} else {
		switch (error) {
		case ERESTART:
			/* redo system call insn */
			tf->tf_pc -= 4;
			break;
		case EJUSTRETURN:
			/* nothing to do */
			break;
		default:
		bad:
#ifndef __HAVE_MINIMAL_EMUL
			if (p->p_emul->e_errno)
				error = p->p_emul->e_errno[error];
#endif
			tf->tf_reg[0] = error;
			tf->tf_spsr |= NZCV_C;
			break;
		}
	}

	userret(l);
}

void EMULNAME(syscall_intern)(struct proc *);

void
EMULNAME(syscall_intern)(struct proc *p)
{
	p->p_md.md_syscall = EMULNAME(syscall);
}
