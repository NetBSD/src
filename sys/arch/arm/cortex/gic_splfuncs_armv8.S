/* $NetBSD: gic_splfuncs_armv8.S,v 1.3 2021/10/31 15:32:14 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "assym.h"

RCSID("$NetBSD: gic_splfuncs_armv8.S,v 1.3 2021/10/31 15:32:14 jmcneill Exp $")

#ifdef __HAVE_PREEMPTION
#define	DISABLE_PREEMPTION			\
	ldr	w1, [x3, #L_NOPREEMPT];		\
	add	w1, w1, #1;			\
	str	w1, [x3, #L_NOPREEMPT]
#define	ENABLE_PREEMPTION			\
	ldr	w1, [x3, #L_NOPREEMPT];		\
	sub	w1, w1, #1;			\
	str	w1, [x3, #L_NOPREEMPT]
#else
#define	DISABLE_PREEMPTION
#define	ENABLE_PREEMPTION
#endif

/*
 * int
 * gic_splraise(int newipl)
 *
 *	w0 = newipl
 */
	.align 7        /* cacheline-aligned */
ENTRY_NP(gic_splraise)
	/* Save curlwp in x3, curcpu in x1 */
	mrs	x3, tpidr_el1			/* get curlwp */
	DISABLE_PREEMPTION
	ldr	x1, [x3, #L_CPU]		/* get curcpu */

	/* If newipl > cpl, update cpl */
	ldr	w2, [x1, #CI_CPL]
	cmp	w0, w2
	b.le	.Lnoraise
	str	w0, [x1, #CI_CPL]

.Lnoraise:
	ENABLE_PREEMPTION
	mov	w0, w2				/* return oldipl */
	ret
END(gic_splraise)


/*
 * void
 * gic_splx(int newipl)
 *
 *	w0 = newipl
 */
	.align 7        /* cacheline-aligned */
ENTRY_NP(gic_splx)
	/* Save curlwp in x3, curcpu in x1 */
	mrs	x3, tpidr_el1			/* get curlwp */
	DISABLE_PREEMPTION
	ldr	x1, [x3, #L_CPU]		/* get curcpu */

.Lagain:
	/* If newipl >= cpl, just return */
	ldr	w2, [x1, #CI_CPL]
	cmp	w0, w2
	b.hs	.Ldone

	/* Slow path if ci_intr_depth != 0 */
	ldr	w2, [x1, #CI_INTR_DEPTH]
	cbnz	w2, .Lslow

	/* Save newipl and restart address in cpu info */
	str	w0, [x1, #CI_SPLX_SAVEDIPL]
	adr	x2, .Lrestart
	str	x2, [x1, #CI_SPLX_RESTART]

	/* Slow path if hwpl > newipl */
	ldr	w2, [x1, #CI_HWPL]
	cmp	w2, w0
	b.hi	.Lrestore

	/* Update cpl */
	str	w0, [x1, #CI_CPL]

	/* Clear saved restart address from cpu info */
	str	xzr, [x1, #CI_SPLX_RESTART]

	/* Check for pending softints */
	ldr	w2, [x1, #CI_SOFTINTS]
	lsr	w2, w2, w0
	cbnz	w2, _C_LABEL(dosoftints)

.Ldone:
	ENABLE_PREEMPTION
	ret

.Lrestart:
	/* Reload registers */
	mrs	x3, tpidr_el1			/* get curlwp */
	ldr	x1, [x3, #L_CPU]		/* get curcpu */
	ldr	w0, [x1, #CI_SPLX_SAVEDIPL]	/* get newipl */
	b	.Lagain
	
.Lrestore:
	/* Clear saved restart address from cpu info */
	str	xzr, [x1, #CI_SPLX_RESTART]

.Lslow:
	ENABLE_PREEMPTION
	/* Jump to slow path */
	b	_C_LABEL(Xgic_splx)
END(gic_splx)
