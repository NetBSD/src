/*	$NetBSD: syscall.c,v 1.13.8.1 2012/04/17 00:06:52 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc. All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)trap.c	7.4 (Berkeley) 5/13/91
 */

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)trap.c	7.4 (Berkeley) 5/13/91
 */

/*
 * SH3 Trap and System call handling
 *
 * T.Horiuchi 1998.06.8
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>

#include <sh3/userret.h>

#include <uvm/uvm_extern.h>

static void syscall_plain(struct lwp *, struct trapframe *);
static void syscall_fancy(struct lwp *, struct trapframe *);

void
syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}


/*
 * System call request from POSIX system call gate interface to kernel.
 *	l  ... curlwp when trap occurs.
 *	tf ... full user context.
 */
static void
syscall_plain(struct lwp *l, struct trapframe *tf)
{
	struct proc *p = l->l_proc;
	void *params;
	const struct sysent *callp;
	int error, opc, nsys;
	size_t argsize;
	register_t code, args[8], rval[2], ocode;

	curcpu()->ci_data.cpu_nsyscall++;

	opc = tf->tf_spc;
	ocode = code = tf->tf_r0;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;
	params = (void *)tf->tf_r15;

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
	        code = tf->tf_r4;  /* fuword(params); */
		/* params += sizeof(int); */
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		if (callp != sysent)
			break;
		/* fuword(params + _QUAD_LOWWORD * sizeof(int)); */
#if _BYTE_ORDER == BIG_ENDIAN
		code = tf->tf_r5;
#else
		code = tf->tf_r4;
#endif
		/* params += sizeof(quad_t); */
		break;
	default:
		break;
	}
	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;
	argsize = callp->sy_argsize;

	if (ocode == SYS_syscall) {
		if (argsize) {
			args[0] = tf->tf_r5;
			args[1] = tf->tf_r6;
			args[2] = tf->tf_r7;
			if (argsize > 3 * sizeof(int)) {
				argsize -= 3 * sizeof(int);
				error = copyin(params, (void *)&args[3],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}
	else if (ocode == SYS___syscall) {
		if (argsize) {
			args[0] = tf->tf_r6;
			args[1] = tf->tf_r7;
			if (argsize > 2 * sizeof(int)) {
				argsize -= 2 * sizeof(int);
				error = copyin(params, (void *)&args[2],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	} else {
		if (argsize) {
			args[0] = tf->tf_r4;
			args[1] = tf->tf_r5;
			args[2] = tf->tf_r6;
			args[3] = tf->tf_r7;
			if (argsize > 4 * sizeof(int)) {
				argsize -= 4 * sizeof(int);
				error = copyin(params, (void *)&args[4],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}

	if (error)
		goto bad;

	rval[0] = 0;
	rval[1] = tf->tf_r1;
	error = sy_call(callp, l, args, rval);

	switch (error) {
	case 0:
		tf->tf_r0 = rval[0];
		tf->tf_r1 = rval[1];
		tf->tf_ssr |= PSL_TBIT;	/* T bit */

		break;
	case ERESTART:
		/* 2 = TRAPA instruction size */
		tf->tf_spc = opc - 2;

		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_r0 = error;
		tf->tf_ssr &= ~PSL_TBIT;	/* T bit */

		break;
	}

	userret(l);
}


/*
 * Like syscall_plain but with trace_enter/trace_exit.
 */
static void
syscall_fancy(struct lwp *l, struct trapframe *tf)
{
	struct proc *p = l->l_proc;
	void *params;
	const struct sysent *callp;
	int error, opc, nsys;
	size_t argsize;
	register_t code, args[8], rval[2], ocode;

	curcpu()->ci_data.cpu_nsyscall++;

	opc = tf->tf_spc;
	ocode = code = tf->tf_r0;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;
	params = (void *)tf->tf_r15;

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
	        code = tf->tf_r4;  /* fuword(params); */
		/* params += sizeof(int); */
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		if (callp != sysent)
			break;
		/* fuword(params + _QUAD_LOWWORD * sizeof(int)); */
#if _BYTE_ORDER == BIG_ENDIAN
		code = tf->tf_r5;
#else
		code = tf->tf_r4;
#endif
		/* params += sizeof(quad_t); */
		break;
	default:
		break;
	}
	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;
	argsize = callp->sy_argsize;

	if (ocode == SYS_syscall) {
		if (argsize) {
			args[0] = tf->tf_r5;
			args[1] = tf->tf_r6;
			args[2] = tf->tf_r7;
			if (argsize > 3 * sizeof(int)) {
				argsize -= 3 * sizeof(int);
				error = copyin(params, (void *)&args[3],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}
	else if (ocode == SYS___syscall) {
		if (argsize) {
			args[0] = tf->tf_r6;
			args[1] = tf->tf_r7;
			if (argsize > 2 * sizeof(int)) {
				argsize -= 2 * sizeof(int);
				error = copyin(params, (void *)&args[2],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	} else {
		if (argsize) {
			args[0] = tf->tf_r4;
			args[1] = tf->tf_r5;
			args[2] = tf->tf_r6;
			args[3] = tf->tf_r7;
			if (argsize > 4 * sizeof(int)) {
				argsize -= 4 * sizeof(int);
				error = copyin(params, (void *)&args[4],
					       argsize);
			} else
				error = 0;
		} else
			error = 0;
	}

	if (error)
		goto bad;

	if ((error = trace_enter(code, args, callp->sy_narg)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = tf->tf_r1;
	error = sy_call(callp, l, args, rval);
out:
	switch (error) {
	case 0:
		tf->tf_r0 = rval[0];
		tf->tf_r1 = rval[1];
		tf->tf_ssr |= PSL_TBIT;	/* T bit */

		break;
	case ERESTART:
		/* 2 = TRAPA instruction size */
		tf->tf_spc = opc - 2;

		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_r0 = error;
		tf->tf_ssr &= ~PSL_TBIT;	/* T bit */

		break;
	}


	trace_exit(code, rval, error);

	userret(l);
}
