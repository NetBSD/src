/*	$NetBSD: linux_exec.h,v 1.14.2.4 2001/09/26 19:54:47 nathanw Exp $	*/

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

#ifndef _LINUX_EXEC_H
#define _LINUX_EXEC_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_exec.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_exec.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_exec.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_exec.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_exec.h>
#else
#error Undefined linux_exec.h machine type.
#endif


/* Defines for a.out executables */
#define LINUX_AOUT_HDR_SIZE (sizeof (struct exec))
#define	LINUX_AOUT_AUX_ARGSIZ	2

#define LINUX_N_MAGIC(ep)    ((ep)->a_midmag & 0xffff)
#define LINUX_N_MACHTYPE(ep) (((ep)->a_midmag >> 16) & 0xff)

#define LINUX_N_TXTOFF(x,m) \
 ((m) == ZMAGIC ? 1024 : ((m) == QMAGIC ? 0 : sizeof (struct exec)))

#define LINUX_N_DATOFF(x,m) (LINUX_N_TXTOFF(x,m) + (x).a_text)

#define LINUX_N_TXTADDR(x,m) ((m) == QMAGIC ? PAGE_SIZE : 0)

#define LINUX__N_SEGMENT_ROUND(x) (((x) + NBPG - 1) & ~(NBPG - 1))

#define LINUX__N_TXTENDADDR(x,m) (LINUX_N_TXTADDR(x,m)+(x).a_text)

#define LINUX_N_DATADDR(x,m) \
    ((m)==OMAGIC? (LINUX__N_TXTENDADDR(x,m)) \
     : (LINUX__N_SEGMENT_ROUND (LINUX__N_TXTENDADDR(x,m))))

#define LINUX_N_BSSADDR(x,m) (LINUX_N_DATADDR(x,m) + (x).a_data)

/* 
 * From Linux's include/linux/elf.h
  */
  #define LINUX_AT_UID    11 /* real uid */
  #define LINUX_AT_EUID   12 /* effective uid */
  #define LINUX_AT_GID    13 /* real gid */
  #define LINUX_AT_EGID   14 /* effective gid */ 
  #define LINUX_AT_PLATFORM 15  /* string identifying CPU for optimizations */
  #define LINUX_AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
  #define LINUX_AT_CLKTCK 17 /* frequency at which times() increments */

#ifdef _KERNEL
__BEGIN_DECLS
extern const struct emul emul_linux;

void linux_setregs __P((struct lwp *, struct exec_package *, u_long));
int exec_linux_aout_makecmds __P((struct proc *, struct exec_package *));
int linux_aout_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));
void linux_trapsignal __P((struct lwp *, int, u_long));

#ifdef EXEC_ELF32
int linux_elf32_probe __P((struct proc *, struct exec_package *, void *,
    char *, vaddr_t *));
#endif
#ifdef EXEC_ELF64
int linux_elf64_probe __P((struct proc *, struct exec_package *, void *,
    char *, vaddr_t *));
#endif
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_LINUX_EXEC_H */
