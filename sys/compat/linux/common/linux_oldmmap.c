/*	$NetBSD: linux_oldmmap.c,v 1.55 2002/02/15 16:48:03 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_oldmmap.c,v 1.55 2002/02/15 16:48:03 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_oldmmap.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

/* Used on: arm, i386, m68k */
/* Not used on: alpha, mips, pcc, sparc, sparc64 */

/*
 * Linux wants to pass everything to a syscall in registers.
 * However mmap() has 6 of them. Oops: out of register error.
 * They just pass everything in a structure.
 */
int
linux_sys_old_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_old_mmap_args /* {
		syscallarg(struct linux_oldmmap *) lmp;
	} */ *uap = v;
	struct linux_oldmmap lmap;
	struct linux_sys_mmap_args nlmap;
	int error;

	if ((error = copyin(SCARG(uap, lmp), &lmap, sizeof lmap)))
		return error;

	SCARG(&nlmap,addr) = (unsigned long)lmap.lm_addr;
	SCARG(&nlmap,len) = lmap.lm_len;
	SCARG(&nlmap,prot) = lmap.lm_prot;
	SCARG(&nlmap,flags) = lmap.lm_flags;
	SCARG(&nlmap,fd) = lmap.lm_fd;
	SCARG(&nlmap,offset) = (unsigned)lmap.lm_pos;
	uprintf("old_mmap(%p, %d, %d, %d, %d, %d)\n",
		lmap.lm_addr, lmap.lm_len, lmap.lm_prot, lmap.lm_flags,
		lmap.lm_fd, lmap.lm_pos);
	return linux_sys_mmap(p, &nlmap, retval);
}

