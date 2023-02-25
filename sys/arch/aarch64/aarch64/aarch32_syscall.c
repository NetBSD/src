/*	$NetBSD: aarch32_syscall.c,v 1.7 2023/02/25 00:40:22 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: aarch32_syscall.c,v 1.7 2023/02/25 00:40:22 riastradh Exp $");

#include <sys/param.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/syscallvar.h>
#include <uvm/uvm_extern.h>

#include <aarch64/armreg.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/userret.h>

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
	bool do_trace, thumbmode;

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_data.cpu_nsyscall++; /* XXX unsafe curcpu() */

	thumbmode = (tf->tf_spsr & SPSR_A32_T) ? true : false;
#ifdef SYSCALL_CODE_REG
	/*
	 * mov.w r<SYSCALL_CODE_REG>, #<syscall_no>
	 * svc #<SYSCALL_CODE_REG_SVC>
	 */
#ifdef SYSCALL_CODE_REG_SVC
	if ((tf->tf_esr & 0xffff) != SYSCALL_CODE_REG_SVC) {
		error = EINVAL;
		goto bad;
	}
#endif
	uint32_t code = tf->tf_reg[SYSCALL_CODE_REG];
#if (SYSCALL_CODE_REG == 0)
	int regstart = 1;		/* args start from r1 */
	int nargs_reg = NARGREG - 1;	/* number of argument in registers */
#else
	int regstart = 0;		/* args start from r0 */
	int nargs_reg = NARGREG;	/* number of argument in registers */
#endif
#else /* SYSCALL_CODE_REG */
	uint32_t code = tf->tf_esr & 0xffff;	/* XXX: 16-23bits are omitted */
	if (thumbmode) {
		if (code != 255) {
			do_trapsignal(l, SIGILL, ILL_ILLTRP,
			    (void *)(tf->tf_pc - 2), tf->tf_esr);
			error = EINVAL;
			goto bad;
		}
		code = tf->tf_reg[0];
		tf->tf_reg[0] = tf->tf_reg[12];	/* orig $r0 is saved to $ip */
	}
	int regstart = 0;		/* args start from r0 */
	int nargs_reg = NARGREG;	/* number of argument in registers */
#endif /* SYSCALL_CODE_REG */

#ifdef SYSCALL_CODE_REMAP
	code = SYSCALL_CODE_REMAP(code);
#endif

	code %= EMULNAMEU(SYS_NSYSENT);
	callp = p->p_emul->e_sysent + code;
#ifndef SYSCALL_NO_INDIRECT
	if (__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
		int off = 1;
#ifdef NETBSD32_SYS_netbsd32____syscall /* XXX ugly: apply only for NETBSD32 */
		/*
		 * For __syscall(2), 1st argument is quad_t, which is
		 * stored in r0 and r1.
		 */
		if (code == NETBSD32_SYS_netbsd32____syscall)
			off = 2;
#endif
		nargs_reg -= off;
		regstart = off;	/* args start from r1 or r2 */
#ifdef __AARCH64EB__
		if (off == 2)
			code = tf->tf_reg[1];
		else
#endif
			code = tf->tf_reg[0];
		code %= EMULNAMEU(SYS_NSYSENT);
		callp = p->p_emul->e_sysent + code;

		/* don't allow nested syscall */
		if (__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
			error = EINVAL;
			goto bad;
		}
	}
#endif /* SYSCALL_NO_INDIRECT */

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

	rval[0] = 0;
	rval[1] = tf->tf_reg[1];
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
	    KDTRACE_ENTRY(callp->sy_entry) ||
	    KDTRACE_ENTRY(callp->sy_return))) {
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
#ifndef SYSCALL_NO_RVAL1
		tf->tf_reg[1] = rval[1];
#endif
		tf->tf_spsr &= ~NZCV_C;
	} else {
		switch (error) {
		case ERESTART:
			/* redo system call insn */
			if (thumbmode)
				tf->tf_pc -= 2;
			else
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
#elif defined(SYSCALL_EMUL_ERRNO)
			error = SYSCALL_EMUL_ERRNO(error);
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
