/*	$NetBSD: darwin_machdep.c,v 1.14.14.1 2009/05/13 17:17:49 jym Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_machdep.c,v 1.14.14.1 2009/05/13 17:17:49 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/mount.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_types.h>
#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscallargs.h>

#include <machine/darwin_machdep.h>

void
darwin_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	printf("darwin_sendsig: sig = %d\n", ksi->ksi_signo);
	return;
}

int
darwin_sys_sigreturn_x2(struct lwp *l, const struct darwin_sys_sigreturn_x2_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_ucontext *) uctx;
	} */

	printf("darwin_sys_sigreturn_x2: uctx = %p\n", SCARG(uap, uctx));
	return 0;
}

/*
 * This is the version used starting with X.3 binaries
 */
int
darwin_sys_sigreturn(struct lwp *l, const struct darwin_sys_sigreturn_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct darwin_ucontext *) uctx;
		syscallarg(int) ucvers;
	} */

	switch (SCARG(uap, ucvers)) {
	case /* DARWIN_UCVERS_X2 */ 1:
		return darwin_sys_sigreturn_x2(l, (const void *)uap, retval);

	default:
		printf("darwin_sys_sigreturn: ucvers = %d\n", 
		    SCARG(uap, ucvers));
		break;
	}
	return 0;
}

/*
 * Set the return value for darwin binaries after a fork(). The userland
 * libSystem stub expects the child pid to be in retval[0] for the parent
 * and the child as well. It will perform the required operation to transform 
 * it in the POSIXly correct value: zero for the child.
 * We also need to skip the next instruction because the system call
 * was successful (We also do this in the syscall handler, Darwin 
 * works that way).
 */
void
darwin_fork_child_return(void *arg)
{
#ifdef notyet
	struct proc * const p = arg;
	struct trapframe * const tf = trapframe(p);

	child_return(arg);

	tf->fixreg[FIRSTARG] = p->p_pid;
	tf->srr0 +=4;
#else
	printf("darwin_fork_child_return: proc = %p\n", arg);
#endif
}
