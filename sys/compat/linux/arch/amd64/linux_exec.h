/*	$NetBSD: linux_exec.h,v 1.3 2005/12/16 14:16:14 christos Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_LINUX_EXEC_H
#define _AMD64_LINUX_EXEC_H

#ifndef ELFSIZE
#define ELFSIZE 64
#endif
#include <sys/exec_elf.h>

/* This is only for a.out, and a.out does not support amd64 */
#define LINUX_M_I386            100
#define LINUX_MID_MACHINE       LINUX_M_I386

#define LINUX_USRSTACK		0x800000000000
#define LINUX_USRSTACK32	0xc0000000

/* Counted from linux_exec_machdep.c */
#define LINUX_ELF_AUX_ENTRIES 16

/* Hardware platform identifier string */
#define LINUX_PLATFORM "x86_64"

/* The extra data (ELF auxiliary table and platform name) on stack */
struct linux_extra_stack_data64 {
	Aux64Info ai[LINUX_ELF_AUX_ENTRIES];
	char pad[1]; /*sizeof(long) - (sizeof(LINUX_PLATFORM) % sizeof(long))*/
	char hw_platform[sizeof(LINUX_PLATFORM)];
};
#define LINUX_ELF_AUX_ARGSIZ sizeof(struct linux_extra_stack_data64)

/* we have special powerpc ELF copyargs */
#define LINUX_MACHDEP_ELF_COPYARGS

int linux_exec_setup_stack(struct lwp *, struct exec_package *);

#endif /* !_AMD64_LINUX_EXEC_H */
