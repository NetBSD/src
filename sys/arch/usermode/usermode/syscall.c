/* $NetBSD: syscall.c,v 1.7 2011/09/08 14:49:42 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.7 2011/09/08 14:49:42 reinoud Exp $");

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

extern void syscall(void);

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
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userland_ucp;
	uint *reg, i;

	l = curlwp;

	printf("syscall called for lwp %p!\n", l);
	reg = (int *) &ucp->uc_mcontext;
#if 1
	/* register dump before call */
	const char *name[] = {"GS", "FS", "ES", "DS", "EDI", "ESI", "EBP", "ESP",
		"EBX", "EDX", "ECX", "EAX", "TRAPNO", "ERR", "EIP", "CS", "EFL",
		"UESP", "SS"};

	for (i =0; i < 19; i++)
		printf("reg[%02d] (%6s) = %"PRIx32"\n", i, name[i], reg[i]);
#endif

	/* system call accounting */
	curcpu()->ci_data.cpu_nsyscall++;

	/* XXX do we want do do emulation? */
	LWP_CACHE_CREDS(l, l->l_proc);
	/* TODO issue!! */

	printf("syscall no. %d\n", reg[11]);
/* skip instruction */
reg[14] += 2;

/* retval */
reg[11] = 0;
	printf("end of syscall : return to userland\n");
	userret(l);
printf("jump back to %p\n", (void *) reg[14]);
}

