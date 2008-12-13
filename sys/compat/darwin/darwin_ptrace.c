/*	$NetBSD: darwin_ptrace.c,v 1.17.6.1 2008/12/13 01:13:47 haad Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_ptrace.c,v 1.17.6.1 2008/12/13 01:13:47 haad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_ptrace.h>
#include <compat/darwin/darwin_syscallargs.h>

#define ISSET(t, f)     ((t) & (f))

int
darwin_sys_ptrace(struct lwp *l, const struct darwin_sys_ptrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(void *) addr;
		syscallarg(int) data;
	} */

	return ENOSYS;	/* code was badly broken */
}

int
darwin_sys_kdebug_trace(struct lwp *l, const struct darwin_sys_kdebug_trace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) debugid;
		syscallarg(int) arg1;
		syscallarg(int) arg2;
		syscallarg(int) arg3;
		syscallarg(int) arg4;
		syscallarg(int) arg5;
	} */
	int args[4];
	char *str;

	args[0] = SCARG(uap, arg1);
	args[1] = SCARG(uap, arg2);
	args[2] = SCARG(uap, arg3);
	args[3] = 0;
	str = (char*)args;

#ifdef DEBUG_DARWIN
	printf("darwin_sys_kdebug_trace(%x, (%x %x %x)/\"%s\", %x, %x)\n",
	    SCARG(uap, debugid), SCARG(uap, arg1), SCARG(uap, arg2),
	    SCARG(uap, arg3), str, SCARG(uap, arg4), SCARG(uap, arg5));
#endif
	return 0;
}
