/*	$NetBSD: syscall.c,v 1.31.16.2 2007/08/03 00:58:02 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
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
/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.31.16.2 2007/08/03 00:58:02 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
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
static void EMULNAME(syscall)(struct lwp *, u_int, u_int, vaddr_t);

register_t MachEmulateBranch(struct frame *, register_t, u_int, int);

#define DELAYBRANCH(x) ((int)(x)<0)

void
EMULNAME(syscall_intern)(struct proc *p)
{
	p->p_md.md_syscall = EMULNAME(syscall);
	p->p_md.md_fancy = trace_is_enabled(p);
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
EMULNAME(syscall)(struct lwp *l, u_int status, u_int cause, vaddr_t opc)
{
	struct proc *p = l->l_proc;
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	register_t *args, copyargs[8];
	register_t *rval = NULL;	/* XXX gcc */
	register_t copyrval[2];
	mips_reg_t ov0;
	size_t nsaved, nargs, argsize;
	const struct sysent *callp;
	int code, error;
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	const int abi = _MIPS_BSD_API_LP32;
	KASSERT(p->p_md.md_abi == abi);
#else
	const int abi = p->p_md.md_abi;
#endif

	LWP_CACHE_CREDS(l, p);

	uvmexp.syscalls++;

	if (DELAYBRANCH(cause))
		frame->f_regs[_R_PC] = MachEmulateBranch(frame, opc, 0, 0);
	else
		frame->f_regs[_R_PC] = opc + sizeof(int);

	callp = p->p_emul->e_sysent;
	ov0 = code = frame->f_regs[_R_V0] - SYSCALL_SHIFT;
	if (abi == _MIPS_BSD_API_N32 || abi == _MIPS_BSD_API_LP64) {
		nsaved = 8;
		argsize = 8;
	} else {
		nsaved = 4;
		argsize = 4;
	}

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		args = copyargs;
		if (code == SYS_syscall
		    || (code == SYS___syscall
			&& (abi == _MIPS_BSD_API_N32
			    || abi == _MIPS_BSD_API_LP64))) {
			/*
			 * Code is first argument, followed by actual args.
			 */
			code = frame->f_regs[_R_A0] - SYSCALL_SHIFT;
			args = &frame->f_regs[_R_A1];
			nsaved--;
		} else {
			/*
			 * Like syscall, but code is a quad, so as to maintain
			 * quad alignment for the rest of the arguments.
			 */
			code = frame->f_regs[_R_A0 + _QUAD_LOWWORD] 
			    - SYSCALL_SHIFT;
			args = &frame->f_regs[_R_A2];
			nsaved = 2;
		}

		if (code >= p->p_emul->e_nsysent)
			callp += p->p_emul->e_nosys;
		else
			callp += code;
		nargs = callp->sy_argsize / sizeof(register_t);

		if (nargs > nsaved) {
			memcpy(copyargs, args, nsaved * sizeof(register_t));
			args = copyargs;
			error = copyin(
			    ((register_t *)(vaddr_t)frame->f_regs[_R_SP] + argsize),
			    (args + nsaved),
			    (nargs - nsaved) * sizeof(register_t));
			if (error)
				goto bad;
		}
		break;

	default:
		if (code >= p->p_emul->e_nsysent)
			callp += p->p_emul->e_nosys;
		else
			callp += code;
		nargs = callp->sy_narg;

		if (nargs <= nsaved) {
			if (abi != _MIPS_BSD_API_LP32_64CLEAN) {
				args = &frame->f_regs[_R_A0];
			} else {
				args = copyargs;
				args[0] = frame->f_regs[_R_A0];
				args[1] = frame->f_regs[_R_A1];
				args[2] = frame->f_regs[_R_A2];
				args[3] = frame->f_regs[_R_A3];
			}
		} else {
			KASSERT(abi != _MIPS_BSD_API_N32);
			KASSERT(abi != _MIPS_BSD_API_LP64);
			args = copyargs;
			error = copyin(
			    ((register_t *)(vaddr_t)frame->f_regs[_R_SP] + argsize),
			    (&copyargs[nsaved]),
			    (nargs - nsaved) * sizeof(register_t));
			if (error)
				goto bad;
			args[0] = frame->f_regs[_R_A0];
			args[1] = frame->f_regs[_R_A1];
			args[2] = frame->f_regs[_R_A2];
			args[3] = frame->f_regs[_R_A3];
		}
		break;
	}

	if (p->p_md.md_fancy
	    && (error = trace_enter(l, code, code, NULL, args)) != 0)
		goto out;

	if (abi != _MIPS_BSD_API_LP32_64CLEAN) {
		rval = &frame->f_regs[_R_V0];
		rval[0] = 0;
		/* rval[1] already has V1 */
	} else {
		rval = copyrval;
		rval[0] = 0;
		rval[1] = frame->f_regs[_R_V1];
	}

	error = (*callp->sy_call)(l, args, rval);

    out:
	switch (error) {
	case 0:
		if (rval != &frame->f_regs[_R_V0]) {
			frame->f_regs[_R_V0] = rval[0];
			frame->f_regs[_R_V1] = rval[1];
		}
		frame->f_regs[_R_A3] = 0;
		break;
	case ERESTART:
		frame->f_regs[_R_V0] = ov0;	/* restore syscall code */
		frame->f_regs[_R_PC] = opc;
		break;
	case EJUSTRETURN:
		break;	/* nothing to do */
	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[_R_V0] = error;
		frame->f_regs[_R_A3] = 1;
		break;
	}

	if (p->p_md.md_fancy)
		trace_exit(l, code, args, rval, error);

	userret(l);
}
