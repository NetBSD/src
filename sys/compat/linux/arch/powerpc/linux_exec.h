/*	$NetBSD: linux_exec.h,v 1.14 2003/03/01 04:36:39 thorpej Exp $  */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	     This product includes software developed by the NetBSD
 *	     Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *	   contributors may be used to endorse or promote products derived
 *	   from this software without specific prior written permission.
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

#ifndef _POWERPC_LINUX_EXEC_H
#define _POWERPC_LINUX_EXEC_H

#include <sys/exec_aout.h>
#include <sys/exec_elf.h>
#include <sys/types.h>

/*
 * From Linux's include/asm-ppc/elf.h
 */
#define LINUX_ELF_HWCAP (0)

/*
 * From Linux's include/asm-ppc/param.h
 */
# define LINUX_CLOCKS_PER_SEC 100	/* frequency at which times() counts */

/*
 * Linux a.out format parameters
 */
#define LINUX_M_POWERPC		MID_POWERPC
#define LINUX_MID_MACHINE	LINUX_M_POWERPC	

/*
 * Linux Elf32 format parameters
 */

#define LINUX_GCC_SIGNATURE 1
/*
 * LINUX_ATEXIT_SIGNATURE enable the atexit_signature test. See 
 * sys/compat/linux/common/linux_exec_elf32.c:linux_atexit_signature()
 */
#define LINUX_ATEXIT_SIGNATURE	1

/*
 * LINUX_SHIFT enable the 16 bytes shift for arguments and ELF auxiliary
 * table. This is needed on the PowerPC
 */
#define LINUX_SHIFT 0x0000000FUL

/* 
 * LINUX_SP_WRAP enable the stack pointer wrap before Linux's ld.so 
 * transfers control to the Linux executable. It is set to the size
 * of the stack pointer wrap code, which is defined in 
 * sys/compat/linux/arch/powerpc/linux_sp_wrap.S
 */
#define LINUX_SP_WRAP 0x30		/* Size of the stack pointer wrap code */

/*
 * Entries in the ELF auxiliary table. This is counted from
 * sys/compat/linux/arc/powerpc/linux_exec_powerpc.c
 */
#define LINUX_ELF_AUX_ENTRIES 14

/*
 * Size of the auxiliary ELF table. On the PowerPC we need 16 extra bytes
 * in order to force an alignement on a 16 bytes boundary (this is expected
 * by PowerPC GNU ld.so). If we use LINUX_SP_WRAP, we also need some extra
 * room for the sp_wrap_code.
 */
#define LINUX_ELF_AUX_ARGSIZ \
    ((howmany(LINUX_ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof(Elf32_Addr))) \
    + 16 + LINUX_SP_WRAP)

/* we have special powerpc ELF copyargs */
#define LINUX_MACHDEP_ELF_COPYARGS

#endif /* !_POWERPC_LINUX_EXEC_H */
