/*	$NetBSD: linux_exec.h,v 1.14.22.1 2014/08/20 00:03:32 tls Exp $ */

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

#ifndef _MIPS_LINUX_EXEC_H
#define _MIPS_LINUX_EXEC_H

#ifdef EXEC_AOUT
#include <sys/exec_aout.h>
#endif
#include <sys/exec_elf.h>
#include <sys/types.h>

/*
 * From Linux's include/asm-mips/elf.h
 */
#define LINUX_ELF_HWCAP (0)

#ifdef EXEC_AOUT
/*
 * Linux a.out format parameters
 */
#define LINUX_M_MIPS		MID_MIPS
#define LINUX_MID_MACHINE	LINUX_M_MIPS
#endif

/*
 * Linux Elf32 format parameters
 */

#define LINUX_ATEXIT_SIGNATURE 1
#define LINUX_GCC_SIGNATURE 1

#define LINUX_ELF_AUX_ARGSIZ \
	(howmany(LINUX_ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof(Elf32_Addr)) + LINUX_RANDOM_BYTES)

#define linux_exec_setup_stack	exec_setup_stack

#endif /* !_MIPS_LINUX_EXEC_H */
