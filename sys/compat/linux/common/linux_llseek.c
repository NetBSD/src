/*	$NetBSD: linux_llseek.c,v 1.29.24.1 2007/12/26 19:49:17 ad Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: linux_llseek.c,v 1.29.24.1 2007/12/26 19:49:17 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>

#include <compat/linux/linux_syscallargs.h>

/* Used on: arm, i386, m68k, mips, ppc, sparc */
/* Not used on: alpha, sparc64 */

/*
 * This appears to be part of a Linux attempt to switch to 64 bits file sizes.
 */
int
linux_sys_llseek(struct lwp *l, const struct linux_sys_llseek_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uint32_t) ohigh;
		syscallarg(uint32_t) olow;
		syscallarg(void *) res;
		syscallarg(int) whence;
	} */
	struct sys_lseek_args bla;
	int error;
	off_t off;

	off = SCARG(uap, olow) | (((off_t) SCARG(uap, ohigh)) << 32);

	SCARG(&bla, fd) = SCARG(uap, fd);
	SCARG(&bla, offset) = off;
	SCARG(&bla, whence) = SCARG(uap, whence);

	if ((error = sys_lseek(l, &bla, retval)))
		return error;

	if ((error = copyout(retval, SCARG(uap, res), sizeof (off_t))))
		return error;

	retval[0] = 0;
	return 0;
}
