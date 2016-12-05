/*	$NetBSD: loadfile.h,v 1.12.36.1 2016/12/05 10:55:26 skrll Exp $	 */

/*-
 * Copyright (c) 1998, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

/*
 * Array indices in the u_long position array
 */
#define MARK_START	0
#define MARK_ENTRY	1
#define MARK_DATA	2
#define MARK_NSYM	3
#define MARK_SYM	4
#define MARK_END	5
#define MARK_MAX	6

/*
 * Bit flags for sections to load
 */
#define LOAD_TEXT	0x0001
#define LOAD_TEXTA	0x0002
#define LOAD_DATA	0x0004
#define LOAD_BSS	0x0008
#define LOAD_SYM	0x0010
#define LOAD_HDR	0x0020
#define LOAD_NOTE	0x0040
#define LOAD_ALL	0x007f
#define LOAD_MINIMAL	0x002f
#define LOAD_BACKWARDS	0x0050

#define COUNT_TEXT	0x0100
#define COUNT_TEXTA	0x0200
#define COUNT_DATA	0x0400
#define COUNT_BSS	0x0800
#define COUNT_SYM	0x1000
#define COUNT_HDR	0x2000
#define COUNT_ALL	0x3f00

int loadfile(const char *, u_long *, int);
int fdloadfile(int fd, u_long *, int);

#ifndef MACHINE_LOADFILE_MACHDEP
#define MACHINE_LOADFILE_MACHDEP "machine/loadfile_machdep.h"
#endif
#include MACHINE_LOADFILE_MACHDEP

#ifdef BOOT_ECOFF
#include <sys/exec_ecoff.h>
int loadfile_coff(int, struct ecoff_exechdr *, u_long *, int);
#endif

#if defined(BOOT_ELF32) || defined(BOOT_ELF64)
#include <sys/exec_elf.h>
#ifdef BOOT_ELF32
int loadfile_elf32(int, Elf32_Ehdr *, u_long *, int);
#endif
#ifdef BOOT_ELF64
int loadfile_elf64(int, Elf64_Ehdr *, u_long *, int);
#endif
#endif /* BOOT_ELF32 || BOOT_ELF64 */

#ifdef BOOT_AOUT
#include <sys/exec_aout.h>
int loadfile_aout(int, struct exec *, u_long *, int);
#endif

extern uint32_t netbsd_version;
extern u_int netbsd_elf_class;
