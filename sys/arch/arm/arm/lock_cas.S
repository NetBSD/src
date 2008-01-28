/*	$NetBSD: lock_cas.S,v 1.1.24.2 2008/01/28 18:14:31 matt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include "opt_arm_debug.h"

#include <machine/asm.h>

	.text
	.align	0

#if defined(ARM_LOCK_CAS_DEBUG)
.L_lock_cas_success:
	.word	_C_LABEL(_lock_cas_success)
.L_lock_cas_fail:
	.word	_C_LABEL(_lock_cas_fail)
#endif /* ARM_LOCK_CAS_DEBUG */

/*
 * _lock_cas:
 *
 *	Perform an atomic compare-and-swap operation.
 *
 *	ARM doesn't have a compare-and-swap, so this is implemented
 *	as a restartable atomic sequence.  See irq_dispatch.S.
 *
 *	Returns true if the swap was performed, false if the swap
 *	was not performed.
 *
 * r0	Address of interest.
 * r1	Old value to compare.
 * r2	New value.
 */
ENTRY_NP(_lock_cas)
#ifdef _ARCH_ARM_6
	.globl	_C_LABEL(_lock_cas_end)
        mov	ip, r0
1:      ldrex	r0, [ip]		/* eventual return value */
        cmp	r1, r0
	RETc(ne)
        strex	r3, r2, [ip]
        cmp	r3, #0
        bne	1b
        RET
        END(_lock_cas)
#else
	ldr	r3, [r0]
	teq	r3, r1
	bne	1f
	str	r2, [r0]

	.globl	_C_LABEL(_lock_cas_end)
_C_LABEL(_lock_cas_end):
#if defined(ARM_LOCK_CAS_DEBUG)
	mov	r3, r1			/* save 'old' */
	ldr	r0, .L_lock_cas_success
	ldmia	r0, {r1-r2}		/* load ev_count */
#if defined(__ARMEB__)
	adds	r2, r2, #1		/* 64-bit incr (lo) */
	adc	r1, r1, #0		/* 64-bit incr (hi) */
#else
	adds	r1, r1, #1		/* 64-bit incr (lo) */
	adc	r2, r2, #0		/* 64-bit incr (hi) */
#endif /* __ARMEB__ */
	stmia	r0, {r1-r2}		/* store ev_count */
	mov	r0, r3			/* return 'old' */
#else
	mov	r0, r1			/* return 'old' */
#endif /* ARM_LOCK_CAS_DEBUG */
	RET
1:
#if defined(ARM_LOCK_CAS_DEBUG)
	ldr	r0, .L_lock_cas_fail
	ldmia	r0, {r1-r2}		/* load ev_count */
#if defined(__ARMEB__)
	adds	r2, r2, #1		/* 64-bit incr (lo) */
	adc	r1, r1, #0		/* 64-bit incr (hi) */
#else
	adds	r1, r1, #1		/* 64-bit incr (lo) */
	adc	r2, r2, #0		/* 64-bit incr (hi) */
#endif /* __ARMEB__ */
	stmia	r0, {r1-r2}		/* store ev_count */
#endif /* ARM_LOCK_CAS_DEBUG */
	mov	r0, r3			/* return actual value */
	RET
#endif

STRONG_ALIAS(_atomic_cas_ulong,_lock_cas)
STRONG_ALIAS(atomic_cas_ulong,_lock_cas)
STRONG_ALIAS(_atomic_cas_32,_lock_cas)
STRONG_ALIAS(atomic_cas_32,_lock_cas)
STRONG_ALIAS(_atomic_cas_uint,_lock_cas)
STRONG_ALIAS(atomic_cas_uint,_lock_cas)
STRONG_ALIAS(_atomic_cas_ptr,_lock_cas)
STRONG_ALIAS(atomic_cas_ptr,_lock_cas)
