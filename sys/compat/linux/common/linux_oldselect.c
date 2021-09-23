/*	$NetBSD: linux_oldselect.c,v 1.59 2021/09/23 06:56:27 ryo Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
__KERNEL_RCSID(0, "$NetBSD: linux_oldselect.c,v 1.59 2021/09/23 06:56:27 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>

#include <sys/sched.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_oldselect.h>

#include <compat/linux/linux_syscallargs.h>

/* Used on: arm, i386, m68k */
/* Not used on: aarch64, alpha, mips, ppc, sparc, sparc64 */

/*
 * Not sure why the arguments to this older version of select() were put
 * into a structure, because there are 5, and that can all be handled
 * in registers on the i386 like Linux wants to.
 */
int
linux_sys_oldselect(struct lwp *l, const struct linux_sys_oldselect_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_oldselect *) lsp;
	} */
	struct linux_oldselect ls;
	int error;

	if ((error = copyin(SCARG(uap, lsp), &ls, sizeof(ls))))
		return error;

	return linux_select1(l, retval, ls.nfds, ls.readfds, ls.writefds,
	    ls.exceptfds, ls.timeout);
}

