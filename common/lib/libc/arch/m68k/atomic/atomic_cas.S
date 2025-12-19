/*	$NetBSD: atomic_cas.S,v 1.16 2025/12/19 03:29:54 thorpej Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#include <sys/ras.h>
#include "atomic_op_asm.h"

	.text

/*
 * For kernel builds on systems with a 68010 or __HAVE_M68K_BROKEN_RMC,
 * then m68k/m68k/lock_stubs.s provides a restartable atomic sequence
 * implementation of _atomic_cas_32() for the kernel's own use.
 *
 * On such systems, _atomic_cas_16() and _atomic_cas_8() are provided
 * by atomic_cas_by_cas32.c.
 */

/* =====> restartable atomic sequence implementation (userspace only) <===== */

#if !defined(_KERNEL)
ENTRY(_atomic_cas_32_ras)
	.hidden _C_LABEL(_atomic_cas_32_ras)

	movl	4(%sp), %a0		/* Fetch ptr */

RAS_START_ASM_HIDDEN(_atomic_cas_32_ras)
	movl	(%a0), %d0		/* d0 = *ptr */
	cmpl	8(%sp), %d0		/* Same as old? */
	jne	1f			/* Nope */
	movl	12(%sp), (%a0)		/* *ptr = new */
RAS_END_ASM_HIDDEN(_atomic_cas_32_ras)
1:	movl	%d0, %a0		/* pointers return also in %a0 */
	rts
END(_atomic_cas_32_ras)

ENTRY(_atomic_cas_16_ras)
	.hidden	_C_LABEL(_atomic_cas_16_ras)

	movl	4(%sp), %a0		/* Fetch ptr */

RAS_START_ASM_HIDDEN(_atomic_cas_16_ras)
	movw	(%a0), %d0		/* d0 = *ptr */
	cmpw	8+2(%sp), %d0		/* Same as old? (lower word) */
	jne	1f			/* Nope */
	movw	12+2(%sp), (%a0)	/* *ptr = new (lower word) */
RAS_END_ASM_HIDDEN(_atomic_cas_16_ras)
1:	rts
END(_atomic_cas_16_ras)

ENTRY(_atomic_cas_8_ras)
	.hidden	_C_LABEL(_atomic_cas_8_ras)

	movl	4(%sp), %a0		/* Fetch ptr */

RAS_START_ASM_HIDDEN(_atomic_cas_8_ras)
	movb	(%a0), %d0		/* d0 = *ptr */
	cmpb	8+3(%sp), %d0		/* Same as old? (lower byte) */
	jne	1f			/* Nope */
	movb	12+3(%sp), (%a0)	/* *ptr = new (lower byte) */
RAS_END_ASM_HIDDEN(_atomic_cas_8_ras)
1:	rts
END(_atomic_cas_8_ras)
#endif /* ! _KERNEL */

/* =====> CASx implementation <===== */

#ifndef __mc68010__
#if !defined(_KERNEL) || !defined(__HAVE_M68K_BROKEN_RMC)
ENTRY(_atomic_cas_32_casl)
	movl	4(%sp), %a0
	movl	8(%sp), %d0
	movl	12(%sp), %d1
	casl	%d0, %d1, (%a0)
	/* %d0 now contains the old value */
	movl	%d0, %a0		/* pointers return also in %a0 */
	rts
END(_atomic_cas_32_casl)

#ifdef _KERNEL
/* Kernel-only aliases; userspace always uses the call stubs. */
STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32_casl)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32_casl)
STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32_casl)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_32,_atomic_cas_32_casl)
CRT_ALIAS(__sync_val_compare_and_swap_4,_atomic_cas_32_casl)

STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_32_casl)

STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_32_casl)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_32_casl)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_32_casl)
STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_32_casl)
#endif /* _KERNEL */

ENTRY(_atomic_cas_16_casw)
	movl	4(%sp), %a0
	movw	8+2(%sp), %d0		/* lower word */
	movw	12+2(%sp), %d1		/* lower word */
	casw	%d0, %d1, (%a0)
	/* %d0 now contains the old value */
	rts
END(_atomic_cas_16_casw)

#ifdef _KERNEL
ATOMIC_OP_ALIAS(atomic_cas_16,_atomic_cas_16_casw)
ATOMIC_OP_ALIAS(_atomic_cas_16,_atomic_cas_16_casw)
CRT_ALIAS(__sync_val_compare_and_swap_2,_atomic_cas_16_casw)
#endif /* _KERNEL */

ENTRY(_atomic_cas_8_casb)
	movl	4(%sp), %a0
	movb	8+3(%sp), %d0		/* lower byte */
	movb	12+3(%sp), %d1		/* lower byte */
	casb	%d0, %d1, (%a0)
	/* %d0 now contains the old value */
	rts
END(_atomic_cas_8_casb)

#ifdef _KERNEL
ATOMIC_OP_ALIAS(atomic_cas_8,_atomic_cas_8_casb)
ATOMIC_OP_ALIAS(_atomic_cas_8,_atomic_cas_8_casb)
CRT_ALIAS(__sync_val_compare_and_swap_1,_atomic_cas_8_casb)
#endif /* _KERNEL */
#endif /* ! _KERNEL || ! __HAVE_M68K_BROKEN_RMC */
#endif /* ! __mc68010__ */
