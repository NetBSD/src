/*	$NetBSD: syscall.c,v 1.1 2002/07/05 13:32:07 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Charles M. Hannum.
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
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_syscall_debug.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <machine/cpu.h>
#include <machine/trap.h>

#include <uvm/uvm_extern.h>

void	syscall_intern(struct proc *);
static void syscall_plain(struct proc *, struct trapframe *);
static void syscall_fancy(struct proc *, struct trapframe *);

void
syscall_intern(struct proc *p)
{

#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET))
		p->p_md.md_syscall = syscall_fancy;
	else
#endif
		p->p_md.md_syscall = syscall_plain;
}

static void
syscall_plain(struct proc *p, struct trapframe *tf)
{
	const struct sysent *callp;
	register_t rval[2];
	register_t copyargs[16];	/* Assumes no syscall has > 16 args */
	register_t code;
	register_t *args;
	int nargs, hidden, error;

	uvmexp.syscalls++;

	code = tf->tf_caller.r0;	/* System call number passed in r0 */
	callp = p->p_emul->e_sysent;

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		code = tf->tf_caller.r2;
		hidden = 1;
		break;

	default:
		hidden = 0;
		break;
	}

	if (code >= p->p_emul->e_nsysent)
		callp += p->p_emul->e_nosys;
	else
		callp += code;

	nargs = callp->sy_narg + hidden;
	switch (nargs) {
	default:
		error = copyin((caddr_t)(uintptr_t)tf->tf_caller.r15,
		    &copyargs[8], (nargs - 8) * sizeof(register_t));
		if (error)
			goto bad;
		copyargs[0] = tf->tf_caller.r2;
		copyargs[1] = tf->tf_caller.r3;
		copyargs[2] = tf->tf_caller.r4;
		copyargs[3] = tf->tf_caller.r5;
		copyargs[4] = tf->tf_caller.r6;
		copyargs[5] = tf->tf_caller.r7;
		copyargs[6] = tf->tf_caller.r8;
		copyargs[7] = tf->tf_caller.r9;
		args = copyargs;
		break;

	case 8:
	case 7:
	case 6:
	case 5:
	case 4:
	case 3:
	case 2:
	case 1:
	case 0:
		args = &tf->tf_caller.r2;
		break;
	}

	args += hidden;

#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif

	rval[0] = 0;
	rval[1] = tf->tf_caller.r3;
	error = (*callp->sy_call)(p, args, rval);
	switch (error) {
	case 0:
		tf->tf_caller.r2 = rval[0];
		tf->tf_caller.r3 = rval[1];
		tf->tf_caller.r0 = 0;	/* Status returned in r0 */
		break;

	case ERESTART:
		tf->tf_state.sf_spc -= 4;
		break;

	case EJUSTRETURN:
		break;

	default:
	bad:
		tf->tf_caller.r2 = error;
		tf->tf_caller.r0 = 1;	/* Status returned in r0 */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval)
#endif
}

static void
syscall_fancy(struct proc *p, struct trapframe *tf)
{
	const struct sysent *callp;
	register_t rval[2];
	register_t copyargs[16];	/* Assumes no syscall has > 16 args */
	register_t code;
	register_t *args;
	int nargs, hidden, error;

	uvmexp.syscalls++;

	code = tf->tf_caller.r0;	/* System call number passed in r0 */
	callp = p->p_emul->e_sysent;

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		code = tf->tf_caller.r2;
		hidden = 1;
		break;

	default:
		hidden = 0;
		break;
	}

	if (code >= p->p_emul->e_nsysent)
		callp += p->p_emul->e_nosys;
	else
		callp += code;

	nargs = callp->sy_narg + hidden;
	switch (nargs) {
	default:
		error = copyin((caddr_t)(uintptr_t)tf->tf_caller.r15,
		    &copyargs[8], (nargs - 8) * sizeof(register_t));
		if (error)
			goto bad;
		copyargs[0] = tf->tf_caller.r2;
		copyargs[1] = tf->tf_caller.r3;
		copyargs[2] = tf->tf_caller.r4;
		copyargs[3] = tf->tf_caller.r5;
		copyargs[4] = tf->tf_caller.r6;
		copyargs[5] = tf->tf_caller.r7;
		copyargs[6] = tf->tf_caller.r8;
		copyargs[7] = tf->tf_caller.r9;
		args = copyargs;
		break;

	case 8:
	case 7:
	case 6:
	case 5:
	case 4:
	case 3:
	case 2:
	case 1:
	case 0:
		args = &tf->tf_caller.r2;
		break;
	}

	args += hidden;

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, callp->sy_argsize, args);
#endif
#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif

	rval[0] = 0;
	rval[1] = tf->tf_caller.r3;
	error = (*callp->sy_call)(p, args, rval);
	switch (error) {
	case 0:
		tf->tf_caller.r2 = rval[0];
		tf->tf_caller.r3 = rval[1];
		tf->tf_caller.r0 = 0;	/* Status returned in r0 */
		break;

	case ERESTART:
		tf->tf_state.sf_spc -= 4;
		break;

	case EJUSTRETURN:
		break;

	default:
	bad:
		tf->tf_caller.r2 = error;
		tf->tf_caller.r0 = 1;	/* Status returned in r0 */
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval)
#endif

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif
}

int
sys_sysarch(struct proc *p, void *v, register_t *retval)
{
#if 0 /* unused */
	struct sysarch_args /* {
		syscallarg(int) op; 
		syscallarg(void *) parms;
	} */ *uap = v;
#endif

	return (ENOSYS);
}

void
child_return(void *arg)
{
	struct proc *p = arg;

	userret(p);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, SYS_fork, 0, 0);
#endif
}
