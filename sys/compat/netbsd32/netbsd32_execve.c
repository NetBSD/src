/*	$NetBSD: netbsd32_execve.c,v 1.32.38.1 2012/02/18 07:33:57 mrg Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: netbsd32_execve.c,v 1.32.38.1 2012/02/18 07:33:57 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

static int
netbsd32_execve_fetch_element(char * const *array, size_t index, char **value)
{
	int error;
	netbsd32_charp const *a32 = (void const *)array;
	netbsd32_charp e;

	error = copyin(a32 + index, &e, sizeof(e));
	if (error)
		return error;
	*value = (char *)NETBSD32PTR64(e);
	return 0;
}

int
netbsd32_execve(struct lwp *l, const struct netbsd32_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */
	const char *path = SCARG_P32(uap, path);

	return execve1(l, path, SCARG_P32(uap, argp),
	    SCARG_P32(uap, envp), netbsd32_execve_fetch_element);
}

int
netbsd32_fexecve(struct lwp *l, const struct netbsd32_fexecve_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */
	struct sys_fexecve_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(argp, char * const);
	NETBSD32TOP_UAP(envp, char * const);

	return sys_fexecve(l, &ua, retval);
}
