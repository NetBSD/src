/*	$NetBSD: linux_oldmmap.c,v 1.66.28.1 2007/12/26 19:49:18 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_oldmmap.c,v 1.66.28.1 2007/12/26 19:49:18 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>

#include <sys/syscallargs.h>

#include <uvm/uvm_param.h>

#ifdef __amd64__
#include <compat/netbsd32/netbsd32.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_machdep.h>
#endif

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_mmap.h>
#include <compat/linux/common/linux_oldmmap.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

/* Used on: arm, i386, m68k */
/* Used for linux32 on: amd64 */
/* Not used on: alpha, mips, pcc, sparc, sparc64 */

#undef DPRINTF
#ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
#else
#define DPRINTF(a)
#endif

/*
 * Linux wants to pass everything to a syscall in registers.
 * However mmap() has 6 of them. Oops: out of register error.
 * They just pass everything in a structure.
 */
int
linux_sys_old_mmap(struct lwp *l, const struct linux_sys_old_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct linux_oldmmap *) lmp;
	} */
	struct linux_oldmmap lmap;
	struct linux_sys_mmap_args nlmap;
	int error;

	if ((error = copyin(SCARG(uap, lmp), &lmap, sizeof lmap)))
		return error;

	if (lmap.lm_offset & PAGE_MASK)
		return EINVAL;

	SCARG(&nlmap,addr) = lmap.lm_addr;
	SCARG(&nlmap,len) = lmap.lm_len;
	SCARG(&nlmap,prot) = lmap.lm_prot;
	SCARG(&nlmap,flags) = lmap.lm_flags;
	SCARG(&nlmap,fd) = lmap.lm_fd;
	SCARG(&nlmap,offset) = lmap.lm_offset;
	DPRINTF(("old_mmap(%#x, %u, %u, %u, %d, %u)\n",
	    lmap.lm_addr, lmap.lm_len, lmap.lm_prot, lmap.lm_flags,
	    lmap.lm_fd, lmap.lm_offset));
	return linux_sys_mmap(l, &nlmap, retval);
}

