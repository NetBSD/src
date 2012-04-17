/*	$NetBSD: syscall.c,v 1.46.2.1 2012/04/17 00:06:40 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and by Charles M. Hannum.
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

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.46.2.1 2012/04/17 00:06:40 yamt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/trap.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/userret.h>

#ifndef EMULNAME
#define EMULNAME(x)	(x)
#endif

#ifndef SYSCALL_SHIFT
#define SYSCALL_SHIFT 0
#endif

void	EMULNAME(syscall_intern)(struct proc *);
static void EMULNAME(syscall)(struct lwp *, uint32_t, uint32_t, vaddr_t);

void
EMULNAME(syscall_intern)(struct proc *p)
{
	p->p_md.md_syscall = EMULNAME(syscall);
}

/*
 * Process a system call.
 *
 * System calls are strange beasts.  They are passed the syscall number
 * in v0, and the arguments in the registers (as normal).  They return
 * an error flag in a3 (if a3 != 0 on return, the syscall had an error),
 * and the return value (if any) in v0 and possibly v1.
 */

void
EMULNAME(syscall)(struct lwp *l, u_int status, u_int cause, vaddr_t pc)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_utf;
	struct reg *reg = &tf->tf_registers;
	mips_reg_t *fargs = &reg->r_regs[_R_A0];
	register_t *args = NULL;
	register_t copyargs[2+SYS_MAXSYSARGS];
	mips_reg_t saved_v0;
	vaddr_t usp;
	size_t nargs;
	const struct sysent *callp;
	int code, error;
#if defined(__mips_o32)
	const int abi = _MIPS_BSD_API_O32;
	KASSERTMSG(p->p_md.md_abi == abi,
	    "pid %d(%p): md_abi(%d) != abi(%d)",
	    p->p_pid, p, p->p_md.md_abi, abi);
	size_t nregs = 4;
#else
	const int abi = p->p_md.md_abi;
	size_t nregs = _MIPS_SIM_NEWABI_P(abi) ? 8 : 4;
	size_t i;
#endif

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_data.cpu_nsyscall++;

	if (cause & MIPS_CR_BR_DELAY)
		reg->r_regs[_R_PC] = mips_emul_branch(tf, pc, 0, false);
	else
		reg->r_regs[_R_PC] = pc + sizeof(uint32_t);

	callp = p->p_emul->e_sysent;
	saved_v0 = code = reg->r_regs[_R_V0];

	code -= SYSCALL_SHIFT;

	if (code == SYS_syscall
	    || (code == SYS___syscall && abi != _MIPS_BSD_API_O32)) {
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = *fargs++ - SYSCALL_SHIFT;
		nregs--;
	} else if (code == SYS___syscall) {
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fargs[_QUAD_LOWWORD] - SYSCALL_SHIFT;
		fargs += 2;
		nregs -= 2;
	}

	if (code >= p->p_emul->e_nsysent)
		callp += p->p_emul->e_nosys;
	else
		callp += code;

	nargs = callp->sy_narg;
	reg->r_regs[_R_V0] = 0;
#if !defined(__mips_o32)
	if (abi != _MIPS_BSD_API_O32) {
#endif
		CTASSERT(sizeof(copyargs[0]) == sizeof(fargs[0]));
		if (nargs <= nregs) {
			/*
			 * Just use the trapframe for the source of arguments
			 */
			args = fargs;
		} else {
			const size_t nsaved = _MIPS_SIM_NEWABI_P(abi) ? 0 : 4;
			KASSERT(nargs <= __arraycount(copyargs));
			args = copyargs;
			/*
			 * Copy the arguments passed via register from the				 * trapframe to our argument array
			 */
			memcpy(copyargs, fargs, nregs * sizeof(register_t));
			/*
			 * Start copying args skipping the register slots
			 * slots on the stack.
			 */
			usp = reg->r_regs[_R_SP] + nsaved*sizeof(register_t);
			error = copyin((register_t *)usp, &copyargs[nregs],
			    (nargs - nregs) * sizeof(copyargs[0]));
			if (error)
				goto bad;
		}
#if !defined(__mips_o32)
	} else do {
		/*
		 * The only difference between O32 and N32 is the calling
		 * sequence.  If you make O32 
		 */
		int32_t copy32args[SYS_MAXSYSARGS];
		int32_t *cargs = copy32args; 
		unsigned int arg64mask = SYCALL_ARG_64_MASK(callp);
		bool doing_arg64;
		size_t narg64 = SYCALL_NARGS64(callp);
		/*
		 * All arguments are 32bits wide and 64bit arguments use
		 * two 32bit registers or stack slots.  We need to remarshall
		 * them into 64bit slots
		 */
		args = copyargs;
		CTASSERT(sizeof(copy32args[0]) != sizeof(fargs[0]));

		/*
		 * If there are no 64bit arguments and all arguments were in
		 * registers, just use the trapframe for the source of arguments
		 */
		if (nargs <= nregs && narg64 == 0) {
			args = fargs;
			break;
		}

		if (nregs <= nargs + narg64) {
			/*
			 * Grab the non-register arguments from the stack
			 * after skipping the slots for the 4 register passed
			 * arguments.
			 */
			usp = reg->r_regs[_R_SP] + 4*sizeof(int32_t);
			error = copyin((int32_t *)usp, copy32args,
			    (nargs + narg64 - nregs) * sizeof(copy32args[0]));
			if (error)
				goto bad;
		}
		/*
		 * Copy all the arguments to copyargs, starting with the ones
		 * in registers.  Using the hints in the 64bit argmask,
		 * we marshall the passed 32bit values into 64bit slots.  If we
		 * encounter a 64 bit argument, we grab two adjacent 32bit
		 * values and synthesize the 64bit argument.
		 */
		for (i = 0, doing_arg64 = false; i < nargs + narg64;) {
			register_t arg;
			if (nregs > 0) {
				arg = (int32_t) *fargs++; 
				nregs--;
			} else {
				arg = *cargs++;
			}
			if (__predict_true((arg64mask & 1) == 0)) {
				/*
				 * Just copy it with sign extension on
				 */
				copyargs[i++] = (int32_t) arg;
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
				copyargs[i] = arg << (_QUAD_LOWWORD*32);
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
				copyargs[i++] |= arg << (_QUAD_HIGHWORD*32);
				arg64mask >>= 1;
			}
		}
	} while (/*CONSTCOND*/ 0);	/* avoid a goto */
#endif

#ifdef MIPS_SYSCALL_DEBUG
	if (p->p_emul->e_syscallnames)
		printf("syscall %s:", p->p_emul->e_syscallnames[code]);
	else
		printf("syscall %u:", code);
	if (nargs == 0)
		printf(" <no args>");
	else for (size_t j = 0; j < nargs; j++) {
		if (j == nregs) printf(" *");
		printf(" [%s%zu]=%#"PRIxREGISTER,
		    SYCALL_ARG_64_P(callp, j) ? "+" : "",
		    j, args[j]);
	}
	printf("\n");
#endif

	if (__predict_false(p->p_trace_enabled)
	    && (error = trace_enter(code, args, nargs)) != 0)
		goto out;

	error = (*callp->sy_call)(l, args, &reg->r_regs[_R_V0]);

    out:
	switch (error) {
	case 0:
#if !defined(__mips_o32)
		if (abi == _MIPS_BSD_API_O32 && SYCALL_RET_64_P(callp)) {
			/*
			 * If this is from O32 and it's a 64bit quantity,
			 * split it into 2 32bit values in adjacent registers.
			 */
			mips_reg_t tmp = reg->r_regs[_R_V0];
			reg->r_regs[_R_V0 + _QUAD_LOWWORD] = (int32_t) tmp;
			reg->r_regs[_R_V0 + _QUAD_HIGHWORD] = tmp >> 32; 
		}
#endif
#ifdef MIPS_SYSCALL_DEBUG
		if (p->p_emul->e_syscallnames)
			printf("syscall %s:", p->p_emul->e_syscallnames[code]);
		else
			printf("syscall %u:", code);
		printf(" return v0=%#"PRIxREGISTER" v1=%#"PRIxREGISTER"\n",
		    reg->r_regs[_R_V0], reg->r_regs[_R_V1]);
#endif
		reg->r_regs[_R_A3] = 0;
		break;
	case ERESTART:
		reg->r_regs[_R_V0] = saved_v0; /* restore syscall code */
		reg->r_regs[_R_PC] = pc;
		break;
	case EJUSTRETURN:
		break;	/* nothing to do */
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		reg->r_regs[_R_V0] = error;
		reg->r_regs[_R_A3] = 1;
#ifdef MIPS_SYSCALL_DEBUG
		if (p->p_emul->e_syscallnames)
			printf("syscall %s:", p->p_emul->e_syscallnames[code]);
		else
			printf("syscall %u:", code);
		printf(" return error=%d\n", error);
#endif
		break;
	}

	if (__predict_false(p->p_trace_enabled))
		trace_exit(code, &reg->r_regs[_R_V0], error);

	KASSERT(l->l_blcnt == 0);
	KASSERT(curcpu()->ci_biglock_count == 0);

	userret(l);
}
