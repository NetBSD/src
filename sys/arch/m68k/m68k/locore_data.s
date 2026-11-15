/*	$NetBSD: locore_data.s,v 1.3 2026/11/15 15:16:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *      #include <m68k/m68k/locore_data.s>
 *
 * THIS MUST BE INCLUDED BEFORE ANY OTHER .data SEGMENT DECLARATIONS.
 */   

#include <machine/asm.h>

#include "assym.h"

	.file	"locore_data.s"

/*
 * The first page of the data segment contains two things:
 *
 * - The primary CPU's cpu_info.
 *
 * - The temporary stack used early during bootstrap.  Not much
 *   stack is needed when tmpstk is being used.  Elsewhere,
 *   it's asserted that cpu_info is < 2K in size, so 4KB should
 *   be quite sufficient.
 */

	.data
	.align	PAGE_SIZE | Should already be true, but this costs nothing.
GLOBAL(cpu_info_primary)
	| cpu_info_store.ci_curlwp = &lwp0;
	.space	CI_CURLWP
	.long	_C_LABEL(lwp0)
	| cpu_info_store.ci_self = &cpu_info_primary;
	.space	CI_SELF-CI_CURLWP-4
	.long	_C_LABEL(cpu_info_primary)
	| cpu_info_store.ci_flags = CPU_F_RUNNING;
#if CI_FLAGS != CI_SELF+4
#error cpu_info layout botch
#endif
	.long	CPU_F_RUNNING
	.space	4096-CI_FLAGS-4
ASGLOBAL(tmpstk)

	.globl	cpuinfo
#ifdef CPUINFO_VA
	_C_LABEL(cpuinfo) = CPUINFO_VA
#else
	_C_LABEL(cpuinfo) = _C_LABEL(cpu_info_primary)
#endif
