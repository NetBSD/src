/*	$NetBSD: darwin_thread.c,v 1.13.28.1 2007/12/26 19:48:52 ad Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_thread.c,v 1.13.28.1 2007/12/26 19:48:52 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>

#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscallargs.h>

#include <machine/darwin_machdep.h>

/*
 * darwin_fork_child_return() sets the return values as expected by Darwin
 * userland: libSystem stub expects the child pid to be in retval[0] for
 * the parent as well as the child.
 */
int
darwin_sys_fork(struct lwp *l, const void *v, register_t *retval)
{
	int error;

	if ((error = fork1(l, 0, SIGCHLD, NULL, 0,
	    darwin_fork_child_return, NULL, retval, NULL)) != 0)
		return error;

	return 0;
}

int
darwin_sys_vfork(struct lwp *l, const void *v, register_t *retval)
{
	int error;

	if ((error = fork1(l, FORK_PPWAIT, SIGCHLD, NULL, 0,
	    darwin_fork_child_return, NULL, retval, NULL)) != 0)
		return error;

	return 0;
}

int
darwin_sys_pthread_exit(struct lwp *l, const struct darwin_sys_pthread_exit_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) value_ptr;
	} */
	struct sys_exit_args cup;
	struct mach_emuldata *med;
	struct proc *p = l->l_proc;
	int error;

	/* Get the status or use zero if it is not possible */
	if ((error = copyin(SCARG(uap, value_ptr), &SCARG(&cup, rval),
	    sizeof(void *))) != 0)
		SCARG(&cup, rval) = 0;

	/* Avoid destroying the parent's rights in mach_e_proc_exit */
	med = (struct mach_emuldata *)p->p_emuldata;
	LIST_INIT(&med->med_right);

	return sys_exit(l, &cup, retval);
}
