/*	$NetBSD: sunos_syscall.c,v 1.21 2009/11/23 00:11:44 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sunos_syscall.c,v 1.21 2009/11/23 00:11:44 rmind Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syslog.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/reg.h>

#include <uvm/uvm_extern.h>

#include <compat/sunos/sunos_syscall.h>
#include <compat/sunos/sunos_exec.h>

void	sunos_syscall_intern(struct proc *);
static void sunos_syscall_plain(register_t, struct lwp *, struct frame *);
static void sunos_syscall_fancy(register_t, struct lwp *, struct frame *);

void
sunos_syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = sunos_syscall_fancy;
	else
		p->p_md.md_syscall = sunos_syscall_plain;
}

static void
sunos_syscall_plain(register_t code, struct lwp *l, struct frame *frame)
{
	struct proc *p = l->l_proc;
	char *params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	/*
	 * SunOS passes the syscall-number on the stack, whereas
	 * BSD passes it in D0. So, we have to get the real "code"
	 * from the stack, and clean up the stack, as SunOS glue
	 * code assumes the kernel pops the syscall argument the
	 * glue pushed on the stack. Sigh...
	 */
	code = fuword((void *)frame->f_regs[SP]);

	/*
	 * XXX
	 * Don't do this for sunos_sigreturn, as there's no stored pc
	 * on the stack to skip, the argument follows the syscall
	 * number without a gap.
	 */
	if (code != SUNOS_SYS_sigreturn) {
		frame->f_regs[SP] += sizeof (int);
		/*
		 * remember that we adjusted the SP,
		 * might have to undo this if the system call
		 * returns ERESTART.
		 */
		l->l_md.md_flags |= MDL_STACKADJ;
	} else
		l->l_md.md_flags &= ~MDL_STACKADJ;

	params = (char *)frame->f_regs[SP] + sizeof(int);

	switch (code) {
	case SUNOS_SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
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
		p = curproc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
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
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}

	/* need new p-value for this */
	if (l->l_md.md_flags & MDL_STACKADJ) {
		l->l_md.md_flags &= ~MDL_STACKADJ;
		if (error == ERESTART)
			frame->f_regs[SP] -= sizeof (int);
	}
}

static void
sunos_syscall_fancy(register_t code, struct lwp *l, struct frame *frame)
{
	struct proc *p = l->l_proc;
	char *params;
	const struct sysent *callp;
	int error, nsys;
	size_t argsize;
	register_t args[16], rval[2];

	nsys = p->p_emul->e_nsysent;
	callp = p->p_emul->e_sysent;

	/*
	 * SunOS passes the syscall-number on the stack, whereas
	 * BSD passes it in D0. So, we have to get the real "code"
	 * from the stack, and clean up the stack, as SunOS glue
	 * code assumes the kernel pops the syscall argument the
	 * glue pushed on the stack. Sigh...
	 */
	code = fuword((void *)frame->f_regs[SP]);

	/*
	 * XXX
	 * Don't do this for sunos_sigreturn, as there's no stored pc
	 * on the stack to skip, the argument follows the syscall
	 * number without a gap.
	 */
	if (code != SUNOS_SYS_sigreturn) {
		frame->f_regs[SP] += sizeof (int);
		/*
		 * remember that we adjusted the SP,
		 * might have to undo this if the system call
		 * returns ERESTART.
		 */
		l->l_md.md_flags |= MDL_STACKADJ;
	} else
		l->l_md.md_flags &= ~MDL_STACKADJ;

	params = (char *)frame->f_regs[SP] + sizeof(int);

	switch (code) {
	case SUNOS_SYS_syscall:
		/*
		 * Code is first argument, followed by actual args.
		 */
		code = fuword(params);
		params += sizeof(int);
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
		 * Reinitialize proc pointer `p' as it may be different
		 * if this is a child returning from fork syscall.
		 */
		p = curproc;
		frame->f_regs[D0] = rval[0];
		frame->f_regs[D1] = rval[1];
		frame->f_sr &= ~PSL_C;	/* carry bit */
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
		frame->f_regs[D0] = error;
		frame->f_sr |= PSL_C;	/* carry bit */
		break;
	}

	/* need new p-value for this */
	if (l->l_md.md_flags & MDL_STACKADJ) {
		l->l_md.md_flags &= ~MDL_STACKADJ;
		if (error == ERESTART)
			frame->f_regs[SP] -= sizeof (int);
	}

	trace_exit(code, rval, error);
}
