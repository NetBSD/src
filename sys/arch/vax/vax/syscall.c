/*	$NetBSD: syscall.c,v 1.16 2009/06/02 23:21:37 pooka Exp $     */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
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

 /* All bugs are subject to removal without further notice */
		
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.16 2009/06/02 23:21:37 pooka Exp $");

#include "opt_multiprocessor.h"
#include "opt_sa.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/ktrace.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>

#include <machine/mtpr.h>
#include <machine/pte.h>
#include <machine/pcb.h>
#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <machine/userret.h>

#ifdef TRAPDEBUG
int startsysc = 0;
#define TDB(a) if (startsysc) printf a
#else
#define TDB(a)
#endif

void syscall(struct trapframe *);

void
syscall_intern(struct proc *p)
{
	p->p_trace_enabled = trace_is_enabled(p);
	p->p_md.md_syscall = syscall;
}

void
syscall(struct trapframe *frame)
{
	int error;
	int rval[2];
	int args[2+SYS_MAXSYSARGS]; /* add two for SYS___syscall + padding */
	struct trapframe * const exptr = frame;
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	const struct emul * const emul = p->p_emul;
	const struct sysent *callp = emul->e_sysent;
	const u_quad_t oticks = p->p_sticks;

	TDB(("trap syscall %s pc %lx, psl %lx, sp %lx, pid %d, frame %p\n",
	    syscallnames[frame->code], frame->pc, frame->psl,frame->sp,
	    p->p_pid,frame));

	uvmexp.syscalls++;
 
 	LWP_CACHE_CREDS(l, p);

	l->l_addr->u_pcb.framep = frame;

	if ((unsigned long) frame->code >= emul->e_nsysent)
		callp += emul->e_nosys;
	else
		callp += frame->code;

	rval[0] = 0;
	rval[1] = frame->r1;

	if (callp->sy_narg) {
		error = copyin((char*)frame->ap + 4, args, callp->sy_argsize);
		if (error)
			goto bad;
	}

#ifdef KERN_SA
	if (__predict_false((l->l_savp)
            && (l->l_savp->savp_pflags & SAVP_FLAG_DELIVERING)))
		l->l_savp->savp_pflags &= ~SAVP_FLAG_DELIVERING;
#endif

	/*
	 * Only trace if tracing is enabled and the syscall isn't indirect
	 * (SYS_syscall or SYS___syscall)
	 */
	if (__predict_true(!p->p_trace_enabled)
	    || __predict_false(callp->sy_flags & SYCALL_INDIRECT)
	    || (error = trace_enter(frame->code, args, callp->sy_narg)) == 0) {
		error = sy_call(callp, curlwp, args, rval);
	}

	KASSERT(exptr == l->l_addr->u_pcb.framep);
	TDB(("return %s pc %lx, psl %lx, sp %lx, pid %d, err %d r0 %d, r1 %d, "
	    "frame %p\n", syscallnames[exptr->code], exptr->pc, exptr->psl,
	    exptr->sp, p->p_pid, error, rval[0], rval[1], exptr));
bad:
	switch (error) {
	case 0:
		exptr->r1 = rval[1];
		exptr->r0 = rval[0];
		exptr->psl &= ~PSL_C;
		break;

	case EJUSTRETURN:
		break;

	case ERESTART:
		/* assumes CHMK $n was used */
		exptr->pc -= (exptr->code > 63 ? 4 : 2);
		break;

	default:
		exptr->r0 = error;
		exptr->psl |= PSL_C;
		break;
	}

	if (__predict_false(p->p_trace_enabled)
	    && __predict_true(!(callp->sy_flags & SYCALL_INDIRECT)))
		trace_exit(frame->code, rval, error);

	userret(l, frame, oticks);
}

void
child_return(void *arg)
{
        struct lwp *l = arg;

	userret(l, l->l_addr->u_pcb.framep, 0);
	ktrsysret(SYS_fork, 0, 0);
}
