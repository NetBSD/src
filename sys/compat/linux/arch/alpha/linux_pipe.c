/*	$NetBSD: linux_pipe.c,v 1.15.14.1 2014/08/20 00:03:31 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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
__KERNEL_RCSID(0, "$NetBSD: linux_pipe.c,v 1.15.14.1 2014/08/20 00:03:31 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>

#include <compat/linux/linux_syscallargs.h>

/*
 * The Alpha version of sys_pipe.
 *
 * Linux returns fd[1] in a4
 * and fd[0] in the retval[0].
 */


int
linux_sys_pipe(struct lwp *l, const void *v, register_t *retval)
{
	int error;

	if ((error = pipe1(l, retval, 0)))
		return error;

	(l->l_md.md_tf)->tf_regs[FRAME_A4] = retval[1];
	return 0;
}

int
linux_sys_pipe2(struct lwp *l, const struct linux_sys_pipe2_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int *) pfds;
		syscallarg(int) flags;
	} */
	int error, flags;

	flags = linux_to_bsd_ioflags(SCARG(uap, flags));
	if ((flags & ~(O_CLOEXEC|O_NONBLOCK)) != 0)
		return EINVAL;

	if ((error = pipe1(l, retval, flags)))
		return error;

	(l->l_md.md_tf)->tf_regs[FRAME_A4] = retval[1];

	return 0;
}
