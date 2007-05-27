/*	$NetBSD: patch.c,v 1.2.8.1 2007/05/27 14:27:06 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Patch kernel code at boot time, depending on available CPU features.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: patch.c,v 1.2.8.1 2007/05/27 14:27:06 ad Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#ifdef i386
#include "opt_cputype.h"
#endif

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>

void	spllower(int);
void	spllower_end(void);
void	i686_spllower(int);
void	i686_spllower_end(void);
void	i686_spllower_patch(void);
void	amd64_spllower(int);
void	amd64_spllower_end(void);
void	amd64_spllower_patch(void);

void	mutex_spin_exit_end(void);
void	i686_mutex_spin_exit(int);
void	i686_mutex_spin_exit_end(void);
void	i686_mutex_spin_exit_patch(void);

void	mb_read(void);
void	mb_read_end(void);
void	mb_write(void);
void	mb_write_end(void);
void	mb_memory(void);
void	mb_memory_end(void);
void	x86_mb_nop(void);
void	x86_mb_nop_end(void);
void	sse2_mb_read(void);
void	sse2_mb_read_end(void);
void	sse2_mb_memory(void);
void	sse2_mb_memory_end(void);

#define	X86_NOP		0x90
#define	X86_REP		0xf3
#define	X86_RET		0xc3
#define	X86_CS		0x2e
#define	X86_DS		0x3e
#define	X86_GROUP_0F	0x0f

static void __attribute__ ((__unused__))
patchfunc(void *from_s, void *from_e, void *to_s, void *to_e,
	  void *pcrel)
{
	uint8_t *ptr;
	u_long psl;

	if ((uintptr_t)from_e - (uintptr_t)from_s !=
	    (uintptr_t)to_e - (uintptr_t)to_s)
		panic("patchfunc: sizes do not match (from=%p)", from_s);

	psl = read_psl();
	disable_intr();
	memcpy(to_s, from_s, (uintptr_t)to_e - (uintptr_t)to_s);
	if (pcrel != NULL) {
		ptr = pcrel;
		/* Branch hints */
		if (ptr[0] == X86_CS || ptr[0] == X86_DS)
			ptr++;
		/* Conditional jumps */
		if (ptr[0] == X86_GROUP_0F)
			ptr++;		
		/* 4-byte relative jump or call */
		*(uint32_t *)(ptr + 1 - (uintptr_t)from_s + (uintptr_t)to_s) +=
		    ((uint32_t)(uintptr_t)from_s - (uint32_t)(uintptr_t)to_s);
	}
	x86_flush();
	write_psl(psl);
}

static inline void  __attribute__ ((__unused__))
patchbytes(void *addr, const int byte1, const int byte2)
{
	((uint8_t *)addr)[0] = (uint8_t)byte1;
	if (byte2 != -1)
		((uint8_t *)addr)[1] = (uint8_t)byte2;
}

void
x86_patch(void)
{
#if !defined(GPROF)
	static int again;
	u_long cr0;

	if (again)
		return;
	again = 1;

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	/*
	 * i686 patches.
	 */
#ifdef I686_CPU
	if (cpu_class == CPUCLASS_686 && (cpu_feature & CPUID_CX8) != 0) {
		patchfunc(
		    i686_spllower, i686_spllower_end,
		    spllower, spllower_end,
		    i686_spllower_patch
		);
#if !defined(LOCKDEBUG) && !defined(I386_CPU) && !defined(DIAGNOSTIC)
		patchfunc(
		    i686_mutex_spin_exit, i686_mutex_spin_exit_end,
		    mutex_spin_exit, mutex_spin_exit_end,
		    i686_mutex_spin_exit_patch
		);
#endif	/* !defined(LOCKDEBUG) && !defined(I386_CPU) */
	}
	if ((cpu_feature & CPUID_SSE2) != 0) {
		patchfunc(
		    sse2_mb_read, sse2_mb_read_end,
		    mb_read, mb_read_end,
		    NULL
		);
		patchfunc(
		    sse2_mb_memory, sse2_mb_memory_end,
		    mb_memory, mb_memory_end,
		    NULL
		);
	}
#endif	/* I686_CPU */

	/*
	 * AMD64 patches.
	 */
#ifdef __x86_64__
	patchfunc(
	    amd64_spllower, amd64_spllower_end,
	    spllower, spllower_end,
	    amd64_spllower_patch
	);
#endif	/* __x86_64__ */

	/*
	 * Uniprocessor.  XXX Should be determined at runtime.
	 */
#if !defined(MULTIPROCESSOR)
	patchfunc(
		x86_mb_nop, x86_mb_nop_end,
		mb_read, mb_read_end,
		NULL
	);
	patchfunc(
		x86_mb_nop, x86_mb_nop_end,
		mb_write, mb_write_end,
		NULL
	);
	patchfunc(
		x86_mb_nop, x86_mb_nop_end,
		mb_memory, mb_memory_end,
		NULL
	);
#endif	/* !MULTIPROCESSOR */

	/* Re-enable write protection. */
	lcr0(cr0);
#endif	/* GPROF */
}
