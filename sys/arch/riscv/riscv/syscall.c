/*	$NetBSD: syscall.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $	*/

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/systm.h>

#include <riscv/locore.h>

#ifndef EMULNAME
#define EMULNAME(x)	(x)
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#endif

#ifndef SYSCALL_SHIFT
#define SYSCALL_SHIFT 0
#endif

void	EMULNAME(syscall_intern)(struct proc *);
static void EMULNAME(syscall)(struct trapframe *);

__CTASSERT(EMULNAME(SYS_MAXSYSARGS) <= 8);

void
EMULNAME(syscall_intern)(struct proc *p)
{
	p->p_md.md_syscall = EMULNAME(syscall);
}

/*
 * Process a system call.
 *
 * System calls are strange beasts.  They are passed the syscall number
 * in t6, and the arguments in the registers (as normal).
 * The return value (if any) in a0 and possibly a1.  The instruction
 * directly after the syscall is excepted to contain a jump instruction 
 * for an error handler.  If the syscall completes with no error, the PC
 * will be advanced past that instruction.
 */

void
EMULNAME(syscall)(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	register_t *args = &tf->tf_a0;
	register_t retval[2];
	const struct sysent *callp;
	int code, error;
	size_t i;
#ifdef _LP64
	const bool pk32_p = (p->p_flag & PK_32) != 0;
	register_t copyargs[EMULNAME(SYS_MAXSYSARGS)];
#endif

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_data.cpu_nsyscall++;

	tf->tf_pc += sizeof(uint32_t);

	callp = p->p_emul->e_sysent;
	code = tf->tf_t6 - SYSCALL_SHIFT;

	/*
	 * Userland should have taken care of moving everything to their
	 * usual place so these code's should never get to the kernel.
	 */
	if (code == SYS_syscall || code == SYS___syscall) {
		error = ENOSYS;
		goto bad;
	}

	if (code >= p->p_emul->e_nsysent)
		callp += p->p_emul->e_nosys;
	else
		callp += code;

	const size_t nargs = callp->sy_narg;
#ifdef _LP64
	/*
	 * If there are no 64bit arguments, we still need "sanitize" the
	 * 32-bit arguments in case they try to slip through a 64-bit pointer.
	 *  and all arguments were in
	 * registers, just use the trapframe for the source of arguments
	 */
	if (pk32_p) {
		size_t narg64 = SYCALL_NARGS64(callp);
		unsigned int arg64mask = SYCALL_ARG_64_MASK(callp);
		bool doing_arg64 = false;
		register_t *args32 = args;

		/*
		 * All arguments are 32bits wide and if we have 64bit arguments
		 * then use two 32bit registers to construct a 64bit argument.
		 * We remarshall them into 64bit slots but we don't want to
		 * disturb the original arguments in case we get restarted.
		 */
		if (SYCALL_NARGS64(callp) > 0) {
			args = copyargs;
		}

		/*
		 * Copy all the arguments to copyargs, starting with the ones
		 * in registers.  Using the hints in the 64bit argmask,
		 * we marshall the passed 32bit values into 64bit slots.  If we
		 * encounter a 64 bit argument, we grab two adjacent 32bit
		 * values and synthesize the 64bit argument.
		 */
		for (i = 0; i < nargs + narg64; ) {
			register_t arg = *args32++; 
			if (__predict_true((arg64mask & 1) == 0)) {
				/*
				 * Just copy it with sign extension on
				 */
				args[i++] = (int32_t) arg;
				arg64mask >>= 1;
				continue;
			}
			/*
			 * 64bit arg.  grab the low 32 bits, discard the high.
			 */
			arg = (uint32_t)arg;
			if (!doing_arg64) {
				/*
				 * Pick up the 1st word of a 64bit arg.
				 * If lowword == 1 then highword == 0,
				 * so this is the highword and thus
				 * shifted left by 32, otherwise
				 * lowword == 0 and highword == 1 so
				 * it isn't shifted at all.  Remember
				 * we still need another word.
				 */
				doing_arg64 = true;
				args[i] = arg << (_QUAD_LOWWORD*32);
				narg64--;	/* one less 64bit arg */
			} else {
				/*
				 * Pick up the 2nd word of a 64bit arg.
				 * if highword == 1, it's shifted left
				 * by 32, otherwise lowword == 1 and
				 * highword == 0 so it isn't shifted at
				 * all.  And now head to the next argument.
				 */
				doing_arg64 = false;
				args[i++] |= arg << (_QUAD_HIGHWORD*32);
				arg64mask >>= 1;
			}
		}
	}
#endif

#ifdef RISCV_SYSCALL_DEBUG
	if (p->p_emul->e_syscallnames)
		printf("syscall %s:", p->p_emul->e_syscallnames[code]);
	else
		printf("syscall %u:", code);
	if (nargs == 0)
		printf(" <no args>");
	else for (size_t j = 0; j < nargs; j++) {
		printf(" [%s%zu]=%#"PRIxREGISTER,
		    SYCALL_ARG_64_P(callp, j) ? "+" : "",
		    j, args[j]);
	}
	printf("\n");
#endif

	error = sy_invoke(callp, l, args, retval, code);

	switch (error) {
	case 0:
#ifdef _LP64
		if (pk32_p && SYCALL_RET_64_P(callp)) {
			/*
			 * If this is from O32 and it's a 64bit quantity,
			 * split it into 2 32bit values in adjacent registers.
			 */
			register_t tmp = retval[0];
			tf->tf_reg[_X_A0 + _QUAD_LOWWORD] = (int32_t) tmp;
			tf->tf_reg[_X_A0 + _QUAD_HIGHWORD] = tmp >> 32; 
		}
#endif
#ifdef RISCV_SYSCALL_DEBUG
		if (p->p_emul->e_syscallnames)
			printf("syscall %s:", p->p_emul->e_syscallnames[code]);
		else
			printf("syscall %u:", code);
		printf(" return a0=%#"PRIxREGISTER" a1=%#"PRIxREGISTER"\n",
		    tf->tf_a0, tf->tf_a1);
#endif
		tf->tf_pc += sizeof(uint32_t);
		break;
	case ERESTART:
		tf->tf_pc -= sizeof(uint32_t);
		break;
	case EJUSTRETURN:
		break;	/* nothing to do */
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_a0 = error;
#ifdef RISCV_SYSCALL_DEBUG
		if (p->p_emul->e_syscallnames)
			printf("syscall %s:", p->p_emul->e_syscallnames[code]);
		else
			printf("syscall %u:", code);
		printf(" return error=%d\n", error);
#endif
		break;
	}

	KASSERT(l->l_blcnt == 0);
	KASSERT(curcpu()->ci_biglock_count == 0);

	userret(l);
}
