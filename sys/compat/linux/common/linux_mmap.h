/*	$NetBSD: linux_mmap.h,v 1.10 2001/09/22 21:24:16 manu Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_MMAP_H
#define _LINUX_MMAP_H

#define LINUX_PROT_NONE		0x00
#define LINUX_PROT_READ		0x01
#define LINUX_PROT_WRITE	0x02
#define LINUX_PROT_EXEC		0x04

#define LINUX_MAP_SHARED	0x0001
#define LINUX_MAP_PRIVATE	0x0002

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_mmap.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_mmap.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_mmap.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_mmap.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_mmap.h>
/*
 * XXX ERH: All below here are guesses.  The header
 * XXX ERH: files are correct but the defined(*)
 * XXX ERH: probably aren't
 *
 * XXX ERH: Can probably drop some of these headers: linux
 * XXX ERH: is stupid about compat stuff.  It doesn't really
 * XXX ERH: use all the defines, but I don't want to go looking
 * XXX ERH: through all the kernel sources right now.
 */
#elif defined(__sparc__)
#include <compat/linux/arch/sparc/linux_mmap.h>
#else
#error Undefined linux_mmap.h machine type.
#endif

/*
 * Ensure the linux_sys_mmap syscall appears to exist:
 */
struct linux_sys_mmap_args {
	syscallarg(unsigned long) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(linux_off_t) offset;
};

#ifdef _KERNEL
__BEGIN_DECLS
int linux_sys_mmap __P((struct proc *p, void *v, register_t *retval));
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_MMAP_H */
