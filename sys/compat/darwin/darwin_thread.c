/*	$NetBSD: darwin_thread.c,v 1.2 2002/12/26 14:41:06 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_thread.c,v 1.2 2002/12/26 14:41:06 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>

#include <sys/syscallargs.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscallargs.h>

#include <machine/darwin_machdep.h>

/* 
 * darwin_fork_child_return() sets the return values as expected by Darwin
 * userland: libSystem stub expects the child pid to be in retval[0] for 
 * the parent as well as the child. 
 */
int
darwin_sys_fork(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	int error;

	if ((error = fork1(p, 0, SIGCHLD, NULL, 0, 
	    darwin_fork_child_return, NULL, retval, NULL)) != 0);
		return error;
	
	return 0;
}

int
darwin_sys_vfork(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	int error;

	if ((error = fork1(p, FORK_PPWAIT, SIGCHLD, NULL, 0, 
	    darwin_fork_child_return, NULL, retval, NULL)) != 0);
		return error;

	return 0;
}

int
darwin_sys_pthread_exit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#ifdef notyet
	struct darwin_sys_pthread_exit_args /* {
		syscallarg(void *) value_ptr;
	} */ *uap = v;
#endif
	/* 
	 * This is called on thread termination. We should make value_ptr
	 * available to other threads doing a join operation. 
	 */
	return 0;
}
