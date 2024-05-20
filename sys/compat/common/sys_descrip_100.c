/*	$NetBSD: sys_descrip_100.c,v 1.2 2024/05/20 09:37:34 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: sys_descrip_100.c,v 1.2 2024/05/20 09:37:34 martin Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_mod.h>

static const struct syscall_package sys_descrip_100_syscalls[] = {
	{ SYS_compat_100_dup3, 0,
	    (sy_call_t *)compat_100_sys_dup3 },
	{ 0, 0, NULL },
};

int
sys_descrip_100_init(void)
{

	return syscall_establish(NULL, sys_descrip_100_syscalls);
}

int
sys_descrip_100_fini(void)
{

	return syscall_disestablish(NULL, sys_descrip_100_syscalls);
}

int
compat_100_sys_dup3(struct lwp *l, const struct compat_100_sys_dup3_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int)	from;
		syscallarg(int)	to;
		syscallarg(int)	flags;
	} */
	return dodup(l, SCARG(uap, from), SCARG(uap, to), SCARG(uap, flags),
	    retval);
}
