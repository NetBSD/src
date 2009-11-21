/* $NetBSD: syscall.c,v 1.35 2009/11/21 05:35:41 rmind Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.35 2009/11/21 05:35:41 rmind Exp $");

#include "opt_sa.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/ktrace.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/alpha.h>
#include <machine/userret.h>

void	syscall_plain(struct lwp *, u_int64_t, struct trapframe *);
void	syscall_fancy(struct lwp *, u_int64_t, struct trapframe *);

void
syscall_intern(struct proc *p)
{

	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}

/*
 * Process a system call.
 *
 * System calls are strange beasts.  They are passed the syscall number
 * in v0, and the arguments in the registers (as normal).  They return
 * an error flag in a3 (if a3 != 0 on return, the syscall had an error),
 * and the return value (if any) in v0.
 *
 * The assembly stub takes care of moving the call number into a register
 * we can get to, and moves all of the argument registers into their places
 * in the trap frame.  On return, it restores the callee-saved registers,
 * a3, and v0 from the frame before returning to the user process.
 */
void
syscall_plain(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
	const struct sysent *callp;
	int error;
	u_int64_t rval[2];
	u_int64_t *args, copyargs[10];				/* XXX */
	u_int hidden, nargs;
	struct proc *p = l->l_proc;

	LWP_CACHE_CREDS(l, p);

	uvmexp.syscalls++;
	l->l_md.md_tf = framep;

	callp = p->p_emul->e_sysent;

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		/*
		 * syscall() and __syscall() are handled the same on
		 * the alpha, as everything is 64-bit aligned, anyway.
		 */
		code = framep->tf_regs[FRAME_A0];
		hidden = 1;
		break;
	default:
		hidden = 0;
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;

	nargs = callp->sy_narg + hidden;
	switch (nargs) {
	default:
		error = copyin((void *)alpha_pal_rdusp(), &copyargs[6],
		    (nargs - 6) * sizeof(u_int64_t));
		if (error)
			goto bad;
	case 6:	
		copyargs[5] = framep->tf_regs[FRAME_A5];
	case 5:	
		copyargs[4] = framep->tf_regs[FRAME_A4];
	case 4:	
		copyargs[3] = framep->tf_regs[FRAME_A3];
		copyargs[2] = framep->tf_regs[FRAME_A2];
		copyargs[1] = framep->tf_regs[FRAME_A1];
		copyargs[0] = framep->tf_regs[FRAME_A0];
		args = copyargs;
		break;
	case 3:	
	case 2:	
	case 1:	
	case 0:
		args = &framep->tf_regs[FRAME_A0];
		break;
	}
	args += hidden;

	rval[0] = 0;
	rval[1] = 0;

	error = sy_call(callp, l, args, rval);

	switch (error) {
	case 0:
		framep->tf_regs[FRAME_V0] = rval[0];
		framep->tf_regs[FRAME_A4] = rval[1];
		framep->tf_regs[FRAME_A3] = 0;
		break;
	case ERESTART:
		framep->tf_regs[FRAME_PC] -= 4;
		break;
	case EJUSTRETURN:
		break;
	default:
	bad:
		framep->tf_regs[FRAME_V0] = error;
		framep->tf_regs[FRAME_A3] = 1;
		break;
	}

	userret(l);
}

void
syscall_fancy(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
	const struct sysent *callp;
	int error;
	u_int64_t rval[2];
	u_int64_t *args, copyargs[10];
	u_int hidden, nargs;
	struct proc *p = l->l_proc;

	LWP_CACHE_CREDS(l, p);

	uvmexp.syscalls++;
	l->l_md.md_tf = framep;

	callp = p->p_emul->e_sysent;

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

	switch (code) {
	case SYS_syscall:
	case SYS___syscall:
		/*
		 * syscall() and __syscall() are handled the same on
		 * the alpha, as everything is 64-bit aligned, anyway.
		 */
		code = framep->tf_regs[FRAME_A0];
		hidden = 1;
		break;
	default:
		hidden = 0;
		break;
	}

	code &= (SYS_NSYSENT - 1);
	callp += code;

	nargs = callp->sy_narg + hidden;
	switch (nargs) {
	default:
		error = copyin((void *)alpha_pal_rdusp(), &copyargs[6],
		    (nargs - 6) * sizeof(u_int64_t));
		if (error) {
			args = copyargs;
			goto bad;
		}
	case 6:	
		copyargs[5] = framep->tf_regs[FRAME_A5];
	case 5:	
		copyargs[4] = framep->tf_regs[FRAME_A4];
	case 4:	
		copyargs[3] = framep->tf_regs[FRAME_A3];
		copyargs[2] = framep->tf_regs[FRAME_A2];
		copyargs[1] = framep->tf_regs[FRAME_A1];
		copyargs[0] = framep->tf_regs[FRAME_A0];
		args = copyargs;
		break;
	case 3:	
	case 2:	
	case 1:	
	case 0:
		args = &framep->tf_regs[FRAME_A0];
		break;
	}
	args += hidden;

	if ((error = trace_enter(code, args, callp->sy_narg)) == 0) {
		rval[0] = 0;
		rval[1] = 0;
		error = sy_call(callp, l, args, rval);
	}

	switch (error) {
	case 0:
		framep->tf_regs[FRAME_V0] = rval[0];
		framep->tf_regs[FRAME_A4] = rval[1];
		framep->tf_regs[FRAME_A3] = 0;
		break;
	case ERESTART:
		framep->tf_regs[FRAME_PC] -= 4;
		break;
	case EJUSTRETURN:
		break;
	default:
	bad:
		framep->tf_regs[FRAME_V0] = error;
		framep->tf_regs[FRAME_A3] = 1;
		break;
	}

	trace_exit(code, rval, error);

	userret(l);
}

/*
 * Process the tail end of a fork() for the child.
 */
void
child_return(void *arg)
{
	struct lwp *l = arg;

	/*
	 * Return values in the frame set by cpu_fork().
	 */

	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}
