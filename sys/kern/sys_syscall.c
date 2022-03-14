/*	$NetBSD: sys_syscall.c,v 1.14 2022/03/14 12:02:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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
__KERNEL_RCSID(0, "$NetBSD: sys_syscall.c,v 1.14 2022/03/14 12:02:19 riastradh Exp $");

#include <sys/syscall_stats.h>
#include <sys/syscallvar.h>

/*
 * MI indirect system call support.
 * Included from sys_indirect.c and compat/netbsd32/netbsd32_indirect.c
 *
 * SYS_SYSCALL is set to the required function name.
 */

#define CONCAT(a,b) __CONCAT(a,b)

static void
CONCAT(SYS_SYSCALL, _biglockcheck)(struct proc *p, int code)
{

#ifdef DIAGNOSTIC
       kpreempt_disable();     /* make curcpu() stable */
       KASSERTMSG(curcpu()->ci_biglock_count == 0,
           "syscall %ld of emul %s leaked %d kernel locks",
           (long)code, p->p_emul->e_name, curcpu()->ci_biglock_count);
       kpreempt_enable();
#endif
}

int
SYS_SYSCALL(struct lwp *l, const struct CONCAT(SYS_SYSCALL, _args) *uap,
    register_t *rval)
{
	/* {
		syscallarg(int) code;
		syscallarg(register_t) args[SYS_MAXSYSARGS];
	} */
	const struct sysent *callp;
	struct proc *p = l->l_proc;
	int code;
	int error;
#ifdef NETBSD32_SYSCALL
	register_t args64[SYS_MAXSYSARGS];
	int i, narg;
	#define TRACE_ARGS args64
#else
	#define TRACE_ARGS &SCARG(uap, args[0])
#endif

	callp = p->p_emul->e_sysent;

	code = SCARG(uap, code) & (SYS_NSYSENT - 1);
	SYSCALL_COUNT(syscall_counts, code);
	callp += code;

	if (__predict_false(callp->sy_flags & SYCALL_INDIRECT))
		return ENOSYS;

	if (__predict_true(!p->p_trace_enabled)) {
		error = sy_call(callp, l, &uap->args, rval);
		CONCAT(SYS_SYSCALL, _biglockcheck)(p, code);
		return error;
	}

#ifdef NETBSD32_SYSCALL
	narg = callp->sy_narg;
	for (i = 0; i < narg; i++)
		args64[i] = SCARG(uap, args[i]);
#endif

	error = trace_enter(code, callp, TRACE_ARGS);
	if (__predict_false(error != 0))
		return error;
	error = sy_call(callp, l, &uap->args, rval);
	trace_exit(code, callp, &uap->args, rval, error);
	CONCAT(SYS_SYSCALL, _biglockcheck)(p, code);
	return error;

	#undef TRACE_ARGS
}
