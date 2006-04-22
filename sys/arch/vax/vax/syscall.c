/*	$NetBSD: syscall.c,v 1.1.8.2 2006/04/22 11:38:08 simonb Exp $     */

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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.1.8.2 2006/04/22 11:38:08 simonb Exp $");

#include "opt_multiprocessor.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/exec.h>
#include <sys/sa.h>
#include <sys/savar.h>
#ifdef  KTRACE
#include <sys/ktrace.h>
#endif
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
volatile int startsysc = 0;
#define TDB(a) if (startsysc) printf a
#else
#define TDB(a)
#endif

void syscall_plain(struct trapframe *);
void syscall_fancy(struct trapframe *);

void
syscall_intern(struct proc *p)
{
	if (trace_is_enabled(p))
		p->p_md.md_syscall = syscall_fancy;
	else
		p->p_md.md_syscall = syscall_plain;
}

void
syscall_plain(struct trapframe *frame)
{
	const struct sysent *callp;
	u_quad_t oticks;
	int nsys;
	int err, rval[2], args[8];
	struct trapframe *exptr;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	TDB(("trap syscall %s pc %lx, psl %lx, sp %lx, pid %d, frame %p\n",
	    syscallnames[frame->code], frame->pc, frame->psl,frame->sp,
	    p->p_pid,frame));
	uvmexp.syscalls++;
 
	exptr = l->l_addr->u_pcb.framep = frame;
	callp = p->p_emul->e_sysent;
	nsys = p->p_emul->e_nsysent;
	oticks = p->p_sticks;

	if (frame->code == SYS___syscall) {
		int g = *(int *)(frame->ap);

		frame->code = *(int *)(frame->ap + 4);
		frame->ap += 8;
		*(int *)(frame->ap) = g - 2;
	}

	if ((unsigned long) frame->code >= nsys)
		callp += p->p_emul->e_nosys;
	else
		callp += frame->code;

	rval[0] = 0;
	rval[1] = frame->r1;
	KERNEL_PROC_LOCK(l);
	if (callp->sy_narg) {
		err = copyin((char*)frame->ap + 4, args, callp->sy_argsize);
		if (err) {
			KERNEL_PROC_UNLOCK(l);
			goto bad;
		}
	}

	err = (*callp->sy_call)(curlwp, args, rval);
	KERNEL_PROC_UNLOCK(l);
	exptr = l->l_addr->u_pcb.framep;

	TDB(("return %s pc %lx, psl %lx, sp %lx, pid %d, err %d r0 %d, r1 %d, "
	    "frame %p\n", syscallnames[exptr->code], exptr->pc, exptr->psl,
	    exptr->sp, p->p_pid, err, rval[0], rval[1], exptr));
bad:
	switch (err) {
	case 0:
		exptr->r1 = rval[1];
		exptr->r0 = rval[0];
		exptr->psl &= ~PSL_C;
		break;

	case EJUSTRETURN:
		break;

	case ERESTART:
		exptr->pc -= (exptr->code > 63 ? 4 : 2);
		break;

	default:
		exptr->r0 = err;
		exptr->psl |= PSL_C;
		break;
	}

	userret(l, frame, oticks);
}

void
syscall_fancy(struct trapframe *frame)
{
	const struct sysent *callp;
	u_quad_t oticks;
	int nsys;
	int err, rval[2], args[8];
	struct trapframe *exptr;
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	TDB(("trap syscall %s pc %lx, psl %lx, sp %lx, pid %d, frame %p\n",
	    syscallnames[frame->code], frame->pc, frame->psl,frame->sp,
	    p->p_pid,frame));
	uvmexp.syscalls++;
 
	exptr = l->l_addr->u_pcb.framep = frame;
	callp = p->p_emul->e_sysent;
	nsys = p->p_emul->e_nsysent;
	oticks = p->p_sticks;

	if (frame->code == SYS___syscall) {
		int g = *(int *)(frame->ap);

		frame->code = *(int *)(frame->ap + 4);
		frame->ap += 8;
		*(int *)(frame->ap) = g - 2;
	}

	if ((unsigned long) frame->code >= nsys)
		callp += p->p_emul->e_nosys;
	else
		callp += frame->code;

	rval[0] = 0;
	rval[1] = frame->r1;
	KERNEL_PROC_LOCK(l);
	if (callp->sy_narg) {
		err = copyin((char*)frame->ap + 4, args, callp->sy_argsize);
		if (err) {
			KERNEL_PROC_UNLOCK(l);
			goto bad;
		}
	}

	if ((err = trace_enter(l, frame->code, frame->code, NULL, args)) != 0)
		goto out;

	err = (*callp->sy_call)(curlwp, args, rval);
out:
	KERNEL_PROC_UNLOCK(l);
	exptr = l->l_addr->u_pcb.framep;
	TDB(("return %s pc %lx, psl %lx, sp %lx, pid %d, err %d r0 %d, r1 %d, "
	    "frame %p\n", syscallnames[exptr->code], exptr->pc, exptr->psl,
	    exptr->sp, p->p_pid, err, rval[0], rval[1], exptr));
bad:
	switch (err) {
	case 0:
		exptr->r1 = rval[1];
		exptr->r0 = rval[0];
		exptr->psl &= ~PSL_C;
		break;

	case EJUSTRETURN:
		break;

	case ERESTART:
		exptr->pc -= (exptr->code > 63 ? 4 : 2);
		break;

	default:
		exptr->r0 = err;
		exptr->psl |= PSL_C;
		break;
	}

	trace_exit(l, frame->code, args, rval, err);

	userret(l, frame, oticks);
}

void
child_return(void *arg)
{
        struct lwp *l = arg;

	KERNEL_PROC_UNLOCK(l);
	userret(l, l->l_addr->u_pcb.framep, 0);

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(l, SYS_fork, 0, 0);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
}
