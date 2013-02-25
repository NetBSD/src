/*	$NetBSD: syscall.c,v 1.21.2.1 2013/02/25 00:29:03 tls Exp $     */

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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.21.2.1 2013/02/25 00:29:03 tls Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/ktrace.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>

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
syscall(struct trapframe *tf)
{
	int error;
	int rval[2];
	int args[2+SYS_MAXSYSARGS]; /* add two for SYS___syscall + padding */
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	const struct emul * const emul = p->p_emul;
	const struct sysent *callp = emul->e_sysent;
	const u_quad_t oticks = p->p_sticks;

	TDB(("trap syscall %s pc %lx, psl %lx, sp %lx, pid %d, frame %p\n",
	    syscallnames[tf->tf_code], tf->tf_pc, tf->tf_psl,tf->tf_sp,
	    p->p_pid,tf));

	curcpu()->ci_data.cpu_nsyscall++;
 
 	LWP_CACHE_CREDS(l, p);

	l->l_md.md_utf = tf;

	if ((unsigned long) tf->tf_code >= emul->e_nsysent)
		callp += emul->e_nosys;
	else
		callp += tf->tf_code;

	rval[0] = 0;
	rval[1] = tf->tf_r1;

	if (callp->sy_narg) {
		error = copyin((char*)tf->tf_ap + 4, args, callp->sy_argsize);
		if (error)
			goto bad;
	}

	/*
	 * Only trace if tracing is enabled and the syscall isn't indirect
	 * (SYS_syscall or SYS___syscall)
	 */
	if (__predict_true(!p->p_trace_enabled)
	    || __predict_false(callp->sy_flags & SYCALL_INDIRECT)
	    || (error = trace_enter(tf->tf_code, args, callp->sy_narg)) == 0) {
		error = sy_call(callp, curlwp, args, rval);
	}

	TDB(("return %s pc %lx, psl %lx, sp %lx, pid %d, err %d r0 %d, r1 %d, "
	    "tf %p\n", syscallnames[tf->tf_code], tf->tf_pc, tf->tf_psl,
	    tf->tf_sp, p->p_pid, error, rval[0], rval[1], tf));
bad:
	switch (error) {
	case 0:
		tf->tf_r1 = rval[1];
		tf->tf_r0 = rval[0];
		tf->tf_psl &= ~PSL_C;
		break;

	case EJUSTRETURN:
		break;

	case ERESTART:
		/* assumes CHMK $n was used */
		tf->tf_pc -= (tf->tf_code > 63 ? 4 : 2);
		break;

	default:
		tf->tf_r0 = error;
		tf->tf_psl |= PSL_C;
		break;
	}

	if (__predict_false(p->p_trace_enabled)
	    && __predict_true(!(callp->sy_flags & SYCALL_INDIRECT)))
		trace_exit(tf->tf_code, rval, error);

	userret(l, tf, oticks);
}

void
child_return(void *arg)
{
	struct lwp *l = arg;

	userret(l, l->l_md.md_utf, 0);
	ktrsysret(SYS_fork, 0, 0);
}

/*
 * Process the tail end of a posix_spawn() for the child.
 */
void
cpu_spawn_return(struct lwp *l)
{

	userret(l, l->l_md.md_utf, 0);
}
