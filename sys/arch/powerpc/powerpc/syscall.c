/*	$NetBSD: syscall.c,v 1.51.2.1 2014/08/20 00:03:20 tls Exp $	*/

/*
 * Copyright (C) 2002 Matt Thomas
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_altivec.h"
#include "opt_multiprocessor.h"
/* DO NOT INCLUDE opt_compat_XXX.h */
/* If needed, they will be included by file that includes this one */

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

#include <powerpc/frame.h>
#include <powerpc/pcb.h>
#include <powerpc/userret.h>

#define	FIRSTARG	3		/* first argument is in reg 3 */
#define	NARGREG		8		/* 8 args are in registers */
#define	MOREARGS(sp)	((void *)((uintptr_t)(sp) + 8)) /* more args go here */

#ifndef EMULNAME
#include <sys/syscall.h>

#define EMULNAME(x)	(x)
#define EMULNAMEU(x)	(x)

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.51.2.1 2014/08/20 00:03:20 tls Exp $");

void
child_return(void *arg)
{
	struct lwp * const l = arg;
	struct trapframe * const tf = l->l_md.md_utf;

	tf->tf_fixreg[FIRSTARG] = 0;
	tf->tf_fixreg[FIRSTARG + 1] = 1;
	tf->tf_cr &= ~0x10000000;
	tf->tf_srr1 &= ~(PSL_FP|PSL_VEC); /* Disable FP & AltiVec, as we can't
					   be them. */
	ktrsysret(SYS_fork, 0, 0);
	/* Profiling?							XXX */
}
#endif

#include <powerpc/spr.h>

static void EMULNAME(syscall)(struct trapframe *);

void
EMULNAME(syscall)(struct trapframe *tf)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	const struct sysent *callp;
	size_t argsize;
	register_t code;
	register_t *params, rval[2];
	register_t args[10];
	int error;
	int n;

	LWP_CACHE_CREDS(l, p);

	curcpu()->ci_ev_scalls.ev_count++;

	code = tf->tf_fixreg[0];
	params = tf->tf_fixreg + FIRSTARG;
	n = NARGREG;

	{
		switch (code) {
		case EMULNAMEU(SYS_syscall):
			/*
			 * code is first argument,
			 * followed by actual args.
			 */
			code = *params++;
			n -= 1;
			break;
#if !defined(COMPAT_LINUX)
		case EMULNAMEU(SYS___syscall):
			params++;
			code = *params++;
			n -= 2;
			break;
#endif
		default:
			break;
		}

		code &= EMULNAMEU(SYS_NSYSENT) - 1;
		callp = p->p_emul->e_sysent + code;
	}

	argsize = callp->sy_argsize;

	if (argsize > n * sizeof(register_t)) {
		memcpy(args, params, n * sizeof(register_t));
		error = copyin(MOREARGS(tf->tf_fixreg[1]),
		    args + n,
		    argsize - n * sizeof(register_t));
		if (error)
			goto bad;
		params = args;
	}

	error = sy_invoke(callp, l, params, rval, code);

	if (__predict_true(error == 0)) {
		tf->tf_fixreg[FIRSTARG] = rval[0];
		tf->tf_fixreg[FIRSTARG + 1] = rval[1];
		tf->tf_cr &= ~0x10000000;
	} else {
		switch (error) {
		case ERESTART:
			/*
			 * Set user's pc back to redo the system call.
			 */
			tf->tf_srr0 -= 4;
			break;
		case EJUSTRETURN:
			/* nothing to do */
			break;
		default:
		bad:
			if (p->p_emul->e_errno)
				error = p->p_emul->e_errno[error];
			tf->tf_fixreg[FIRSTARG] = error;
			tf->tf_cr |= 0x10000000;
			break;
		}
	}

	userret(l, tf);
}

void EMULNAME(syscall_intern)(struct proc *);

void
EMULNAME(syscall_intern)(struct proc *p)
{

	p->p_md.md_syscall = EMULNAME(syscall);
}
