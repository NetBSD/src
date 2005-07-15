/*	$NetBSD: syscall.c,v 1.7 2005/07/15 09:00:15 martin Exp $ */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)trap.c	8.4 (Berkeley) 9/23/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.7 2005/07/15 09:00:15 martin Exp $");

#define NEW_FPSTATE

#include "opt_syscall_debug.h"
#include "opt_ktrace.h"
#include "opt_systrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/user.h>
#include <sys/signal.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#ifdef SYSTRACE
#include <sys/systrace.h>
#endif
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/instr.h>
#include <machine/pmap.h>
#include <machine/frame.h>
#include <machine/userret.h>

#include <sparc/fpu/fpu_extern.h>
#include <sparc64/sparc64/cache.h>

#ifndef offsetof
#define	offsetof(s, f) ((size_t)&((s *)0)->f)
#endif
#define	MAXARGS	8

union args {
	register32_t i[MAXARGS];
	register64_t l[MAXARGS];
	register_t   r[MAXARGS];
};

static __inline int handle_old(struct trapframe64 *, register_t *);
static __inline int getargs(struct proc *, struct trapframe64 *,
    register_t *, const struct sysent **, union args *, int *);
void syscall_plain(struct trapframe64 *, register_t, register_t);
void syscall_fancy(struct trapframe64 *, register_t, register_t);

/*
 * Handle old style system calls.
 */
static __inline int
handle_old(struct trapframe64 *tf, register_t *code)
{
	int new = *code & (SYSCALL_G7RFLAG | SYSCALL_G2RFLAG);
	*code &= ~(SYSCALL_G7RFLAG | SYSCALL_G2RFLAG);
	if (new)
		tf->tf_pc = tf->tf_global[new & SYSCALL_G2RFLAG ? 2 : 7];
	else
		tf->tf_pc = tf->tf_npc;
	return new;
}


/*
 * The first six system call arguments are in the six %o registers.
 * Any arguments beyond that are in the `argument extension' area
 * of the user's stack frame (see <machine/frame.h>).
 *
 * Check for ``special'' codes that alter this, namely syscall and
 * __syscall.  The latter takes a quad syscall number, so that other
 * arguments are at their natural alignments.  Adjust the number
 * of ``easy'' arguments as appropriate; we will copy the hard
 * ones later as needed.
 */
static __inline int
getargs(struct proc *p, struct trapframe64 *tf, register_t *code,
    const struct sysent **callp, union args *args, int *s64)
{
	int64_t *ap = &tf->tf_out[0];
	int i, error, nap = 6;
	*s64 = tf->tf_out[6] & 1L; /* Do we have a 64-bit stack? */

	*callp = p->p_emul->e_sysent;
	switch (*code) {
	case SYS_syscall:
		*code = *ap++;
		nap--;
		break;
	case SYS___syscall:
		if (*s64) {
			/* longs *are* quadwords */
			*code = ap[0];
			ap += 1;
			nap -= 1;			
		} else {
			*code = ap[_QUAD_LOWWORD];
			ap += 2;
			nap -= 2;
		}
		break;
	}

	if (*code < 0 || *code >= p->p_emul->e_nsysent)
		return ENOSYS;

	*callp += *code;

	if (*s64) {
		/* 64-bit stack -- not really supported on 32-bit kernels */
		register64_t *argp;
#ifdef DEBUG
#ifdef __arch64__
		if ((p->p_flag & P_32) != 0) {
			printf("syscall(): 64-bit stack but P_32 set\n");
			Debugger();
		}
#else
		printf("syscall(): 64-bit stack on a 32-bit kernel????\n");
		Debugger();
#endif
#endif
		i = (*callp)->sy_narg;
		if (__predict_false(i > nap)) {	/* usually false */
			void *pos = (char *)(u_long)tf->tf_out[6] + BIAS +
			   offsetof(struct frame64, fr_argx);
#ifdef DIAGNOSTIC
			KASSERT(i <= MAXARGS);
#endif
			/* Read the whole block in */
			error = copyin(pos, &args->l[nap],
			    (i - nap) * sizeof(*argp));
			if (error)
				return error;
			i = nap;
		}
		for (argp = args->l; i--;) 
			*argp++ = *ap++;
	} else {
		register32_t *argp;

		i = (long)(*callp)->sy_argsize / sizeof(register32_t);
		if (__predict_false(i > nap)) {	/* usually false */
			void *pos = (char *)(u_long)tf->tf_out[6] +
			    offsetof(struct frame32, fr_argx);
#ifdef DIAGNOSTIC
			KASSERT(i <= MAXARGS);
#endif
			/* Read the whole block in */
			error = copyin(pos, &args->i[nap],
			    (i - nap) * sizeof(*argp));
			if (error)
				return error;
			i = nap;
		}
		/* Need to convert from int64 to int32 or we lose */
		for (argp = args->i; i--;) 
			*argp++ = *ap++;
	}
	return 0;
}

void
syscall_intern(struct proc *p)
{
#ifdef KTRACE
	if (p->p_traceflag & (KTRFAC_SYSCALL | KTRFAC_SYSRET)) {
		p->p_md.md_syscall = syscall_fancy;
		return;
	}
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE)) {
		p->p_md.md_syscall = syscall_fancy;
		return;
	} 
#endif
	p->p_md.md_syscall = syscall_plain;
}

/*
 * System calls.  `pc' is just a copy of tf->tf_pc.
 *
 * Note that the things labelled `out' registers in the trapframe were the
 * `in' registers within the syscall trap code (because of the automatic
 * `save' effect of each trap).  They are, however, the %o registers of the
 * thing that made the system call, and are named that way here.
 *
 * 32-bit system calls on a 64-bit system are a problem.  Each system call
 * argument is stored in the smaller of the argument's true size or a
 * `register_t'.  Now on a 64-bit machine all normal types can be stored in a
 * `register_t'.  (The only exceptions would be 128-bit `quad's or 128-bit
 * extended precision floating point values, which we don't support.)  For
 * 32-bit syscalls, 64-bit integers like `off_t's, double precision floating
 * point values, and several other types cannot fit in a 32-bit `register_t'.
 * These will require reading in two `register_t' values for one argument.
 *
 * In order to calculate the true size of the arguments and therefore whether
 * any argument needs to be split into two slots, the system call args
 * structure needs to be built with the appropriately sized register_t.
 * Otherwise the emul needs to do some magic to split oversized arguments.
 *
 * We can handle most this stuff for normal syscalls by using either a 32-bit
 * or 64-bit array of `register_t' arguments.  Unfortunately ktrace always
 * expects arguments to be `register_t's, so it loses badly.  What's worse,
 * ktrace may need to do size translations to massage the argument array
 * appropriately according to the emulation that is doing the ktrace.
 *  
 */
void
syscall_plain(struct trapframe64 *tf, register_t code, register_t pc)
{
	const struct sysent *callp;
	struct lwp *l = curlwp;
	union args args;
#ifdef SYSCALL_DEBUG
	union args *ap;
#ifdef __arch64__
	union args args64;
	int i;
#endif
#endif
	struct proc *p = l->l_proc;
	int error, new;
	register_t rval[2];
	u_quad_t sticks;
	vaddr_t opc, onpc;
	int s64;

	uvmexp.syscalls++;
	sticks = p->p_sticks;
	l->l_md.md_tf = tf;

	/*
	 * save pc/npc in case of ERESTART
	 * adjust pc/npc to new values
	 */
	opc = tf->tf_pc;
	onpc = tf->tf_npc;

	new = handle_old(tf, &code);

	tf->tf_npc = tf->tf_pc + 4;

	if ((error = getargs(p, tf, &code, &callp, &args, &s64)) != 0)
		goto bad;

#ifdef SYSCALL_DEBUG
#ifdef __arch64__
	if (s64)
		ap = &args;
	else {
		for (i = 0; i < callp->sy_narg; i++)
			args64.l[i] = args.i[i];
		ap = &args64;
	}
#else
	ap = &args;
#endif
	scdebug_call(l, code, ap->r);
#endif /* SYSCALL_DEBUG */

	rval[0] = 0;
	rval[1] = tf->tf_out[1];

        /* Lock the kernel if the syscall isn't MP-safe. */
	if (callp->sy_flags & SYCALL_MPSAFE) {
		error = (*callp->sy_call)(l, &args, rval);
	} else {
		KERNEL_PROC_LOCK(l);
		error = (*callp->sy_call)(l, &args, rval);
		KERNEL_PROC_UNLOCK(l);
	}

	switch (error) {
	case 0:
		/* Note: fork() does not return here in the child */
		tf->tf_out[0] = rval[0];
		tf->tf_out[1] = rval[1];
		if (!new)
			/* old system call convention: clear C on success */
			tf->tf_tstate &= ~(((int64_t)(ICC_C | XCC_C)) <<
					   TSTATE_CCR_SHIFT);	/* success */
		break;

	case ERESTART:
		tf->tf_pc = opc;
		tf->tf_npc = onpc;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_out[0] = error;
		tf->tf_tstate |= (((int64_t)(ICC_C | XCC_C)) <<
		    TSTATE_CCR_SHIFT);	/* fail */
		tf->tf_pc = onpc;
		tf->tf_npc = tf->tf_pc + 4;
		break;
	}

#ifdef SYSCALL_DEBUG
	scdebug_ret(l, code, error, rval);
#endif /* SYSCALL_DEBUG */

	userret(l, pc, sticks);
	share_fpu(l, tf);
}

void
syscall_fancy(struct trapframe64 *tf, register_t code, register_t pc)
{
	const struct sysent *callp;
	struct lwp *l = curlwp;
	union args args, *ap = NULL; /* XXX gcc */
#ifdef __arch64__
	union args args64;
	int i;
#endif
	struct proc *p = l->l_proc;
	int error, new;
	register_t rval[2];
	u_quad_t sticks;
	vaddr_t opc, onpc;
	int s64;

	uvmexp.syscalls++;
	sticks = p->p_sticks;
	l->l_md.md_tf = tf;

	/*
	 * save pc/npc in case of ERESTART
	 * adjust pc/npc to new values
	 */
	opc = tf->tf_pc;
	onpc = tf->tf_npc;

	new = handle_old(tf, &code);

	tf->tf_npc = tf->tf_pc + 4;

	if ((error = getargs(p, tf, &code, &callp, &args, &s64)) != 0)
		goto bad;
		
#ifdef __arch64__
	if (s64)
		ap = &args;
	else {
		for (i = 0; i < callp->sy_narg; i++)
			args64.l[i] = args.i[i];
		ap = &args64;
	}
#else
	ap = &args;
#endif
	KERNEL_PROC_LOCK(l);
	if ((error = trace_enter(l, code, code, NULL, ap->r)) != 0) {
		KERNEL_PROC_UNLOCK(l);
		goto out;
	}
#ifdef __arch64__
	if (!s64)
		for (i = 0; i < callp->sy_narg; i++)
			args.i[i] = args64.l[i];
#endif

	rval[0] = 0;
	rval[1] = tf->tf_out[1];

	if (callp->sy_flags & SYCALL_MPSAFE) {
		KERNEL_PROC_UNLOCK(l);
		error = (*callp->sy_call)(l, &args, rval);
	} else {
		error = (*callp->sy_call)(l, &args, rval);
		KERNEL_PROC_UNLOCK(l);
	}
out:
	switch (error) {
	case 0:
		/* Note: fork() does not return here in the child */
		tf->tf_out[0] = rval[0];
		tf->tf_out[1] = rval[1];
		if (!new)
			/* old system call convention: clear C on success */
			tf->tf_tstate &= ~(((int64_t)(ICC_C | XCC_C)) <<
			    TSTATE_CCR_SHIFT);	/* success */
		break;

	case ERESTART:
		tf->tf_pc = opc;
		tf->tf_npc = onpc;
		break;

	case EJUSTRETURN:
		/* nothing to do */
		break;

	default:
	bad:
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		tf->tf_out[0] = error;
		tf->tf_tstate |= (((int64_t)(ICC_C | XCC_C)) <<
				  TSTATE_CCR_SHIFT);	/* fail */
		tf->tf_pc = onpc;
		tf->tf_npc = tf->tf_pc + 4;
		break;
	}

	trace_exit(l, code, ap->r, rval, error);

	userret(l, pc, sticks);
	share_fpu(l, tf);
}

/*
 * Process the tail end of a fork() for the child.
 */
void
child_return(arg)
	void *arg;
{
	struct lwp *l = arg;
#ifdef KTRACE
	struct proc *p = l->l_proc;
#endif

	/*
	 * Return values in the frame set by cpu_lwp_fork().
	 */
	KERNEL_PROC_UNLOCK(l);
	userret(l, l->l_md.md_tf->tf_pc, 0);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p,
			  (p->p_flag & P_PPWAIT) ? SYS_vfork : SYS_fork, 0, 0);
#endif
}



/* 
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curlwp;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	KERNEL_PROC_UNLOCK(l);
	userret(l, 0, 0);
}

void
upcallret(struct lwp *l)
{

	KERNEL_PROC_UNLOCK(l);
	userret(l, 0, 0);
}
