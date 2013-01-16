/*	$NetBSD: netbsd32_wait.c,v 1.21.12.1 2013/01/16 05:33:12 yamt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_wait.c,v 1.21.12.1 2013/01/16 05:33:12 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32___wait450(struct lwp *l, const struct netbsd32___wait450_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */
	int error, status, pid = SCARG(uap, pid);
	struct netbsd32_rusage ru32;
	struct rusage ru;

	error = do_sys_wait(&pid, &status, SCARG(uap, options),
	    SCARG_P32(uap, rusage) != NULL ? &ru : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	if (SCARG_P32(uap, rusage)) {
		netbsd32_from_rusage(&ru, &ru32);
		error = copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
	}

	if (error == 0 && SCARG_P32(uap, status))
		error = copyout(&status, SCARG_P32(uap, status), sizeof(status));

	return error;
}


int
netbsd32___getrusage50(struct lwp *l,
    const struct netbsd32___getrusage50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */
	int error;
	struct proc *p = l->l_proc;
	struct rusage ru;
	struct netbsd32_rusage ru32;

	error = getrusage1(p, SCARG(uap, who), &ru);
	if (error != 0)
		return error;

	netbsd32_from_rusage(&ru, &ru32);
	return copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
}
