/*	$NetBSD: linux32_exec.h,v 1.4.18.1 2014/08/20 00:03:32 tls Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
#ifndef _AMD64_LINUX32_EXEC_H
#define _AMD64_LINUX32_EXEC_H

#include <sys/exec_elf.h>

#define LINUX32_M_I386		100
#define LINUX32_MID_MACHINE	LINUX_M_I386
#define LINUX32_USRSTACK	0xC0000000

#define LINUX32_DEBUGLINK_SIGNATURE	1

/* Counted from common/linux32_exec_elf32.c */
#define LINUX32_ELF_AUX_ENTRIES 15

#define LINUX32_RANDOM_BYTES 16		/* 16 bytes for AT_RANDOM */

#if 0

/* Hardware platform identifier string */
#define LINUX32_PLATFORM "i686" 

#define LINUX32_CPUCAP (cpu_feature[0])

/* vsyscall assembly */
static char linux32_kernel_vsyscall[] = {
	0x55,				/* push   %ebp */		\
	0x89, 0xcd,			/* mov    %ecx,%ebp */ 		\
	0x0f, 0x05,			/* syscall */			\
	0xb9, 0x7b, 0x00, 0x00, 0x00,	/* mov    $0x7b,%ecx */		\
	0x8e, 0xd1,			/* movl   %ecx,%ss */		\
	0x89, 0xe9,			/* mov    %ebp,%ecx */		\
	0x5d,				/* pop    %ebp */		\
	0xc3,				/* ret */			\
};
 
/* The extra data (ELF auxiliary table and platform name) on stack */  
struct linux32_extra_stack_data {
        Aux32Info ai[LINUX32_ELF_AUX_ENTRIES];
        char hw_platform[sizeof(LINUX32_PLATFORM)];
	int pad;
	Elf32_Ehdr elfhdr;
	char kernel_vsyscall[sizeof(linux32_kernel_vsyscall)];
};      
#define LINUX32_ELF_AUX_ARGSIZ sizeof(struct linux32_extra_stack_data)

#endif

#define LINUX32_ELF_AUX_ARGSIZ \
	(howmany(LINUX32_ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof(Elf32_Addr)) + LINUX32_RANDOM_BYTES)

#ifdef _KERNEL
int linux32_exec_setup_stack(struct lwp *, struct exec_package *);
#endif

#endif /* !_AMD64_LINUX32_EXEC_H */
