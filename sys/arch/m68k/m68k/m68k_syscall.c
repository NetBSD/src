/*	$NetBSD: m68k_syscall.c,v 1.43 2010/10/15 10:20:09 tsutsui Exp $	*/

/*-
 * Portions Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: trap.c 1.37 92/12/20$
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/4/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: trap.c 1.37 92/12/20$
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_syscall.c,v 1.43 2010/10/15 10:20:09 tsutsui Exp $");

#include "opt_execfmt.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_aout_m68k.h"
#include "opt_sa.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syslog.h>
#include <sys/ktrace.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/reg.h>

#include <uvm/uvm_extern.h>

/*
 * Defined in machine-specific code (usually trap.c)
 * XXX: This will disappear when all m68k ports share common trap() code...
 */
extern void machine_userret(struct lwp *, struct frame *, u_quad_t);

void syscall(register_t, struct frame);

void	aoutm68k_syscall_intern(struct proc *);
static void syscall_plain(register_t, struct lwp *, struct frame *);
static void syscall_fancy(register_t, struct lwp *, struct frame *);


/*
 * Process a system call.
 */
void
syscall(register_t code, struct frame frame)
{
	struct lwp *l;
	struct proc *p;
	u_quad_t sticks;

	uvmexp.syscalls++;
	if (!USERMODE(frame.f_sr))
		panic("syscall");

	l = curlwp;
	p = l->l_proc;
	sticks = p->p_sticks;
	l->l_md.md_regs = frame.f_regs;
	LWP_CACHE_CREDS(l, p);

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

	(p->p_md.md_syscall)(code, l, &frame);

	machine_userret(l, &frame, sticks);
}

void
syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}

/*
 * Not worth the effort of a whole new set of syscall_{plain,fancy} functions
 */
void
aoutm68k_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}

static void
syscall_plain(register_t code, struct lwp *l, struct frame *frame)
{
	char *params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];
	struct proc *p = l->l_proc;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (char *)frame->f_regs[SP] + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
#if defined(COMPAT_13) || defined(COMPAT_16)
		/*
		 * XXX sigreturn requires special stack manipulation
		 * that is only done if entered via the sigreturn
		 * trap.  Cannot allow it here so make sure we fail.
		 */
		switch (code) {
#ifdef COMPAT_13
		case SYS_compat_13_sigreturn13:
#endif
#ifdef COMPAT_16
		case SYS_compat_16___sigreturn14:
#endif
			code = nsys;
			break;
		}
#endif
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;

	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (void *)args, argsize);
		if (error)
			goto bad;
	}

	rval[0] = 0;
	rval[1] = frame->f_regs[D1];
	error = sy_call(callp, l, args, rval);

	switch (error) {
	case 0:
		/*
		 * Reinitialize proc pointer `p' as it may be different
		 * if this is a child returning from fork syscall.
		 */
		l = curlwp;
		p = l->l_proc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
#ifdef COMPAT_50
		/*
		 * Starting with the 5.0 release all libc assembler
		 * stubs properly handle returning pointers in %a0
		 * themselves, so no need to copy the syscall return
		 * value there. However, -current binaries post 4.0
		 * but pre-5.0 might still require this copy, so we
		 * select this behaviour based on COMPAT_50 as we have
		 * no equivalent for the exact in-between version.
		 */
#ifdef COMPAT_AOUT_M68K
		{
			extern struct emul emul_netbsd_aoutm68k;

			/*
			 * Some pre-m68k ELF libc assembler stubs assume
			 * %a0 is preserved across system calls...
			 */
			if (p->p_emul != &emul_netbsd_aoutm68k)
				frame->f_regs[A0] = rval[0];
		}
#else
		frame->f_regs[A0] = rval[0];
#endif
#endif
		break;
	case ERESTART:
		/*
		 * We always enter through a `trap' instruction, which is 2
		 * bytes, so adjust the pc by that amount.
		 */
		frame->f_pc = frame->f_pc - 2;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		/*
		 * XXX: SVR4 uses this code-path, so we may have
		 * to translate errno.
		 */
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}
}

static void
syscall_fancy(register_t code, struct lwp *l, struct frame *frame)
{
	char *params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];
	struct proc *p = l->l_proc;

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	params = (char *)frame->f_regs[SP] + sizeof(int);

	switch (code) {
	case SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
#if defined(COMPAT_13) || defined(COMPAT_16)
		/*
		 * XXX sigreturn requires special stack manipulation
		 * that is only done if entered via the sigreturn
		 * trap.  Cannot allow it here so make sure we fail.
		 */
		switch (code) {
#ifdef COMPAT_13
		case SYS_compat_13_sigreturn13:
#endif
#ifdef COMPAT_16
		case SYS_compat_16___sigreturn14:
#endif
			code = nsys;
			break;
		}
#endif
		break;
	case SYS___syscall:
		/*
		 * Like syscall, but code is a quad, so as to maintain
		 * quad alignment for the rest of the arguments.
		 */
		code = fuword(params + _QUAD_LOWWORD * sizeof(int));
		params += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (code < 0 || code >= nsys)
		callp += p->p_emul->e_nosys;		/* illegal */
	else
		callp += code;

	argsize = callp->sy_argsize;
	if (argsize) {
		error = copyin(params, (void *)args, argsize);
		if (error)
			goto bad;
	}

	if ((error = trace_enter(code, args, callp->sy_narg)) != 0)
		goto out;

	rval[0] = 0;
	rval[1] = frame->f_regs[D1];
	error = sy_call(callp, l, args, rval);
out:
	switch (error) {
	case 0:
		/*
		 * Reinitialize lwp/proc pointers as they may be different
		 * if this is a child returning from fork syscall.
		 */
		l = curlwp;
		p = l->l_proc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
#ifdef COMPAT_50
		/* see syscall_plain for a comment explaining this */
#ifdef COMPAT_AOUT_M68K
		{
			extern struct emul emul_netbsd_aoutm68k;

			/*
			 * Some pre-m68k ELF libc assembler stubs assume
			 * %a0 is preserved across system calls...
			 */
			if (p->p_emul != &emul_netbsd_aoutm68k)
				frame->f_regs[A0] = rval[0];
		}
#else
		frame->f_regs[A0] = rval[0];
#endif
#endif
		break;
	case ERESTART:
		/*
		 * We always enter through a `trap' instruction, which is 2
		 * bytes, so adjust the pc by that amount.
		 */
		frame->f_pc = frame->f_pc - 2;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		/*
		 * XXX: SVR4 uses this code-path, so we may have
		 * to translate errno.
		 */
		if (p->p_emul->e_errno)
			error = p->p_emul->e_errno[error];
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}

	trace_exit(code, rval, error);
}

void
child_return(void *arg)
{
	struct lwp *l = arg;
	/* See cpu_lwp_fork() */
	struct frame *f = (struct frame *)l->l_md.md_regs;

	f->f_regs[D0] = 0;
	f->f_sr &= ~PSL_C;
	f->f_format = FMT0;

	machine_userret(l, f, 0);
	ktrsysret(SYS_fork, 0, 0);
}

/*
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	lwp_t *l = curlwp;
	struct frame *f = (struct frame *)l->l_md.md_regs;
	int error;

	f->f_regs[D0] = 0;
	f->f_sr &= ~PSL_C;
	f->f_format = FMT0;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	machine_userret(l, f, 0);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	struct frame *f = (struct frame *)l->l_md.md_regs;

	machine_userret(l, f, 0);
}
