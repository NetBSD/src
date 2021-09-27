/*	$NetBSD: syscall.c,v 1.10 2021/09/27 17:40:39 ryo Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

#include <aarch64/userret.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>

#ifndef NARGREG
#define	NARGREG		8		/* 8 args are in registers */
#endif
#define	MOREARGS(sp)	((const void *)(uintptr_t)(sp)) /* more args go here */
#ifndef REGISTER_T
#define REGISTER_T	register_t
#endif

#ifndef EMULNAME
#include <sys/syscall.h>

#define SYSCALL_INDIRECT_CODE_REG	17	/* netbsd/aarch64 use x17 */

#define EMULNAME(x)	(x)
#define EMULNAMEU(x)	(x)

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.10 2021/09/27 17:40:39 ryo Exp $");

void
cpu_spawn_return(struct lwp *l)
{

	userret(l);
}

void
md_child_return(struct lwp *l)
{
	struct trapframe * const tf = lwp_trapframe(l);

	tf->tf_reg[0] = 0;
	tf->tf_reg[1] = 1;
	tf->tf_spsr &= ~NZCV_C;
	l->l_md.md_cpacr = CPACR_FPEN_NONE;

	userret(l);
}
#endif

static void EMULNAME(syscall)(struct trapframe *);

void
EMULNAME(syscall)(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	register_t rval[2];
	register_t args[10];
	int error;

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_data.cpu_nsyscall++;

	register_t *params = (void *)tf->tf_reg;
	size_t nargs = NARGREG;
#ifdef SYSCALL_CODE_REG
	/*
	 * mov x<SYSCALL_CODE_REG>, #<syscall_no>
	 * svc #<optional>
	 */
	size_t code = tf->tf_reg[SYSCALL_CODE_REG];
#if (SYSCALL_CODE_REG == 0)
	params++;
#endif
#else /* SYSCALL_CODE_REG */
	/*
	 * svc #<syscall_no>
	 */
	size_t code = tf->tf_esr & 0xffff;
#endif /* SYSCALL_CODE_REG */

#ifndef SYSCALL_NO_INDIRECT
	switch (code) {
	case EMULNAMEU(SYS_syscall):
	case EMULNAMEU(SYS___syscall):
#if (SYSCALL_INDIRECT_CODE_REG == 0)
		code = *params++;
		nargs -= 1;
#else
		code = tf->tf_reg[SYSCALL_INDIRECT_CODE_REG];
#endif
		/*
		 * code is first argument,
		 * followed by actual args.
		 */
		break;
	default:
		break;
	}
#endif /* !SYSCALL_NO_INDIRECT */

	code &= EMULNAMEU(SYS_NSYSENT) - 1;
	const struct sysent * const callp = p->p_emul->e_sysent + code;

	if (__predict_false(callp->sy_narg > nargs)) {
		const size_t diff = callp->sy_narg - nargs;
		memcpy(args, params, nargs * sizeof(params[0]));
		if (sizeof(params[0]) == sizeof(REGISTER_T)) {
			error = copyin(MOREARGS(tf->tf_sp), &args[nargs],
			    diff * sizeof(register_t));
			if (error)
				goto bad;
		} else {
			/*
			 * If the register_t used by the process isn't the
			 * as the one used by the kernel, we can't directly
			 * copy the arguments off the stack into args.  We
			 * need to buffer them in a REGISTER_T array and
			 * then move them individually into args.
			 */
			REGISTER_T args32[10];
			error = copyin(MOREARGS(tf->tf_sp), args32,
			    diff * sizeof(REGISTER_T));
			if (error)
				goto bad;
			for (size_t i = 0; i < diff; i++) {
				args[nargs + i] = args32[i];
			}
		}
		params = args;
	}

#ifdef __AARCH64EB__
#define SYCALL_ARG_64(a, b)	(((register_t) (a) << 32) | (uint32_t)(b))
#else
#define SYCALL_ARG_64(a, b)	(((register_t) (b) << 32) | (uint32_t)(a))
#endif

	/*
	 * If the syscall used a different (smaller) register size,
	 * reconstruct the 64-bit arguments from two 32-bit registers.
	 */
	if (__predict_false(sizeof(register_t) != sizeof(REGISTER_T)
			    && SYCALL_NARGS64(callp) > 0)) {
		for (size_t i = 0, j = 0; i < callp->sy_narg; i++, j++) {
			if (SYCALL_ARG_64_P(callp, i)) {
				register_t *inp = &params[j];
				args[i] = SYCALL_ARG_64(inp[0], inp[1]);
				j++;
			} else if (i != j) {
				args[i] = params[j];
			}
		}
		params = args;
	}

	rval[0] = 0;
	rval[1] = tf->tf_reg[1];
	error = sy_invoke(callp, l, params, rval, code);

	if (__predict_true(error == 0)) {
		if (__predict_false(sizeof(register_t) != sizeof(REGISTER_T)
				    && SYCALL_RET_64_P(callp))) {
#ifdef __AARCH64EB__
			tf->tf_reg[0] = (uint32_t) (rval[0] >> 32);
			tf->tf_reg[1] = (uint32_t) (rval[0] >>  0);
#else
			tf->tf_reg[0] = (uint32_t) (rval[0] >>  0);
			tf->tf_reg[1] = (uint32_t) (rval[0] >> 32);
#endif
		} else {
			tf->tf_reg[0] = rval[0];
#ifndef SYSCALL_NO_RVAL1
			tf->tf_reg[1] = rval[1];
#endif
		}
		tf->tf_spsr &= ~NZCV_C;
	} else {
		switch (error) {
		case ERESTART:
			/*
			 * Set user's pc back to redo the system call.
			 */
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
