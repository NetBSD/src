/*	$NetBSD: prom_vectors.S,v 1.1 2026/06/11 00:21:35 rumble Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>
#include <machine/asm.h>

RCSID("$NetBSD: prom_vectors.S,v 1.1 2026/06/11 00:21:35 rumble Exp $");

	.text
	.set reorder

/*
 * Calls a PROM vector and ensures MIPS_CURLWP is restored in case the PROM
 * method clobbers it.
 *
 * Some vectors, like reset, will never return and so MIPS_CURLWP doesn't need
 * to be saved. But we just use the same trampoline for simplicity/consistency.
 */
NESTED(sgi_prom_4orless_args, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, CALLFRAME_SIZ

	REG_S	ra, CALLFRAME_RA(sp)
	REG_S	s0, CALLFRAME_SP(sp)

	move	s0, MIPS_CURLWP
	jalr	t0

	move	MIPS_CURLWP, s0

	REG_L	ra, CALLFRAME_RA(sp)
	REG_L	s0, CALLFRAME_SP(sp)

	PTR_ADDU sp, CALLFRAME_SIZ
	jr	ra
END(sgi_prom_4orless_args)

#define	PROMV4ORLESS(vector, name)				\
	.globl	__CONCAT(sgi_prom_,name);			\
NESTED(__CONCAT(sgi_prom_,name), 0, ra);			\
	li	t0, vector;					\
	b	sgi_prom_4orless_args;				\
END(__CONCAT(sgi_prom_,name))

/*
 * Pre-ARCS SGI PROM vectors in KSEG1.
 *
 * The PROM apparently starts at the same address on all pre-ARCS machines. It
 * seems to have derived from MIPSCO and shares some common vector offsets, but
 * most are unimplemented. Especially annoying is that there appear to be no
 * disk I/O routines.
 *
 * Not all vectors are implemented on all machines.
 */

/* These appear to always be implemented (IP4, IP5, IP6, IP12, IP17). */
#define SGI_PROM_RESET     0xbfc00000
#define SGI_PROM_EXEC      0xbfc00008
#define SGI_PROM_RESTART   0xbfc00010
#define SGI_PROM_REINIT    0xbfc00018		// "Okay to power off..." prompt
#define SGI_PROM_AUTOBOOT  0xbfc00028
#define SGI_PROM_PRINTF    0xbfc00080

/* These appear to be implemented in some machines but not others. */
#define SGI_PROM_EADDR     0xbfc001a8		// Only IP6, IP12?
#define SGI_PROM_DEXEC     0xbfc001b8		// Only IP6, IP12?
#define SGI_PROM_NVRAM_TAB 0xbfc001d0		// Only IP12?

/* Vectors we currently use: */
PROMV4ORLESS(SGI_PROM_RESET,  reset)
PROMV4ORLESS(SGI_PROM_REINIT, reinit)
PROMV4ORLESS(SGI_PROM_PRINTF, printf_direct)	// use sgi_prom_printf() instead
