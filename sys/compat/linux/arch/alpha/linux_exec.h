/*	$NetBSD: linux_exec.h,v 1.3.2.1 2001/08/03 04:12:42 lukem Exp $	*/

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

#ifndef _ALPHA_LINUX_EXEC_H
#define _ALPHA_LINUX_EXEC_H

#include <sys/exec_aout.h>
#include <sys/exec_elf.h>

#define LINUX_M_ALPHA		MID_ALPHA
#define LINUX_MID_MACHINE	LINUX_M_ALPHA

#define LINUX_COPYARGS_FUNCTION	ELFNAME2(linux,copyargs)

/*
 * Alpha specific ELF defines.
 *
 * If this is common to more than one linux port move it
 * to common/linux_exec.h.
 */
#define LINUX_AT_NOTELF  	10
#define	LINUX_AT_UID      	11
#define LINUX_AT_EUID     	12
#define LINUX_AT_GID      	13
#define	LINUX_AT_EGID     	14
#define LINUX_AT_PLATFORM	15
#define LINUX_AT_HWCAP    	16

#define LINUX_ELF_AUX_ENTRIES 12

#define LINUX_ELF_AUX_ARGSIZ howmany(sizeof(LinuxAuxInfo) * LINUX_ELF_AUX_ENTRIES, sizeof(char *))

typedef struct {
	Elf32_Sword	a_type;
	Elf32_Word	a_v;
} LinuxAux32Info;

typedef struct {
	Elf64_Word	a_type;
	Elf64_Word	a_v;
} LinuxAux64Info;

#if defined(ELFSIZE) && (ELFSIZE == 32)
#define LinuxAuxInfo	LinuxAux32Info
#else
#define LinuxAuxInfo	LinuxAux64Info
#endif


#ifdef _KERNEL
__BEGIN_DECLS
#ifdef EXEC_ELF32
int linux_elf32_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));
#endif
#ifdef EXEC_ELF64
int linux_elf64_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));
#endif
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_ALPHA_LINUX_EXEC_H */
