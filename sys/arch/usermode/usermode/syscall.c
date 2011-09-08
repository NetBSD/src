/* $NetBSD: syscall.c,v 1.8 2011/09/08 19:39:00 reinoud Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.8 2011/09/08 19:39:00 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/sched.h>
#include <sys/ktrace.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <sys/userret.h>
#include <machine/pcb.h>
#include <machine/thunk.h>
#include <machine/machdep.h>


void userret(struct lwp *l);

void
userret(struct lwp *l)
{
	/* invoke MI userret code */
	mi_userret(l);
}

void
child_return(void *arg)
{
	lwp_t *l = arg;
//	struct pcb *pcb = lwp_getpcb(l);

	/* XXX? */
//	frame->registers[0] = 0;

	printf("child return! lwp %p\n", l);
	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

void
syscall(void)
{	
	lwp_t *l = curlwp;
	const struct proc * const p = l->l_proc;
	const struct sysent *callp;
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userland_ucp;
	register_t copyargs[2+SYS_MAXSYSARGS];
	register_t *args;
	register_t rval[2];
	uint32_t code;
	uint nargs, argsize;
	int error;

//	printf("syscall called for lwp %p!\n", l);

	/* system call accounting */
	curcpu()->ci_data.cpu_nsyscall++;
	LWP_CACHE_CREDS(l, l->l_proc);

	/* XXX do we want do do emulation? */
	md_syscall_get_syscallnumber(ucp, &code);
	code &= (SYS_NSYSENT -1);

	callp   = p->p_emul->e_sysent + code;
	nargs   = callp->sy_narg;
	argsize = callp->sy_argsize;

	printf("syscall no. %d, ", code);
	printf("nargs %d, argsize %d =>  ", nargs, argsize);

	args  = copyargs;
	rval[0] = rval[1] = 0;
	error = md_syscall_getargs(l, ucp, nargs, argsize, args);
	if (!error) 
		error = (*callp->sy_call)(l, args, rval);

	printf("error = %d, rval[0] = %"PRIx32", retval[1] = %"PRIx32"\n",
		error, (uint) rval[0], (uint) rval[1]);
	switch (error) {
	default:
		rval[0] = error;
//		rval[1] = 0;
		/* fall trough */
	case 0:
		md_syscall_set_returnargs(l, ucp, rval);
		/* fall trough */
	case EJUSTRETURN:
		md_syscall_inc_pc(ucp);
		break;
	case ERESTART:
		/* nothing to do */
		break;
	}

//	printf("end of syscall : return to userland\n");
	userret(l);
}

