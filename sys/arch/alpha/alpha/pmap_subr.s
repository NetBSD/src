/* $NetBSD: pmap_subr.s,v 1.1 2021/07/10 20:22:37 thorpej Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(7, "$NetBSD: pmap_subr.s,v 1.1 2021/07/10 20:22:37 thorpej Exp $")

/*
 * Optimized pmap subroutines.
 */

	.text
inc7:	.stabs	__FILE__,132,0,0,inc7;	.loc	1 __LINE__

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified (machine independent) page by mapping the page
 *	into virtual memory and clear its contents, one machine dependent
 *	page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
	.p2align 4
LEAF(pmap_zero_page, 1)
	/* No global references - skip LDGP() */

	/*
	 * Code here is arranged to keep branch targets on 16-byte
	 * boundaries, minimize result latencies in the loop, unroll
	 * the loop to at least 20 insns, and to dual-issue when
	 * feasible.
	 *
	 * In the setup, we use nop and unop to minimize pipline stalls
	 * on dependent instruction pairs.
	 */

	/* ---- */
	lda	t0, -1
	nop
	sll	t0, 42, t0		/* t0 = ALPHA_K0SEG_BASE */
	/*
	 * Loop counter:
	 * PAGE_SIZE / 8 bytes per store / 16 stores per iteration
	 */
	lda	v0, ((ALPHA_PGBYTES / 8) / 16)
	/* ---- */
	or	a0, t0, a0		/* a0 = ALPHA_PHYS_TO_K0SEG(a0) */
	nop
	addq	a0, (8*8), a2		/* a2 = a0 + 8-quads */
	unop
	/* ---- */
1:	stq	zero, (0*8)(a0)		/* 0 */
	stq	zero, (1*8)(a0)		/* 1 */
	stq	zero, (2*8)(a0)		/* 2 */
	stq	zero, (3*8)(a0)		/* 3 */
	/* ---- */
	stq	zero, (4*8)(a0)		/* 4 */
	stq	zero, (5*8)(a0)		/* 5 */
	stq	zero, (6*8)(a0)		/* 6 */
	stq	zero, (7*8)(a0)		/* 7 */
	/* ---- */
	addq	a2, (8*8), a0		/* a0 = a2 + 8-quads */
	stq	zero, (0*8)(a2)		/* 8 */
	stq	zero, (1*8)(a2)		/* 9 */
	stq	zero, (2*8)(a2)		/* 10 */
	/* --- */
	subq	v0, 1, v0		/* count-- */
	stq	zero, (3*8)(a2)		/* 11 */
	stq	zero, (4*8)(a2)		/* 12 */
	stq	zero, (5*8)(a2)		/* 13 */
	/* ---- */
	stq	zero, (6*8)(a2)		/* 14 */
	stq	zero, (7*8)(a2)		/* 15 */
	addq	a0, (8*8), a2		/* a2 = a0 + 8-quads */
	bne	v0, 1b			/* loop around if count != 0 */
	/* ---- */

	RET
	END(pmap_zero_page)

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified (machine independent) page by mapping the page
 *	into virtual memory and copying the page, one machine dependent
 *	page at a time.
 *
 *	Note: no locking is necessary in this function.
 */
	.p2align 4
LEAF(pmap_copy_page, 2)
	/* No global references - skip LDGP() */

	/* See above. */

	/* ---- */
	lda	t0, -1
	nop
	sll	t0, 42, t0		/* t0 = ALPHA_K0SEG_BASE */
	/*
	 * Loop counter:
	 * PAGE_SIZE / 8 bytes per store / 8 stores per iteration
	 */
	lda	v0, ((ALPHA_PGBYTES / 8) / 8)
	/* ---- */
	or	a0, t0, a0		/* a0 = ALPHA_PHYS_TO_K0SEG(a0) */
	unop
	or	a1, t0, a1		/* a1 = ALPHA_PHYS_TO_K0SEG(a1) */
	unop
	/* ---- */
1:	ldq	t0, (0*8)(a0)		/* load 0 */
	ldq	t1, (1*8)(a0)		/* load 1 */
	ldq	t2, (2*8)(a0)		/* load 2 */
	ldq	t3, (3*8)(a0)		/* load 3 */
	/* ---- */
	ldq	t4, (4*8)(a0)		/* load 4 */
	ldq	t5, (5*8)(a0)		/* load 5 */
	ldq	t6, (6*8)(a0)		/* load 6 */
	ldq	t7, (7*8)(a0)		/* load 7 */
	/* ---- */
	addq	a0, (8*8), a0		/* a0 = a0 + 8-quads */
	stq	t0, (0*8)(a1)		/* store 0 */
	stq	t1, (1*8)(a1)		/* store 1 */
	stq	t2, (2*8)(a1)		/* store 2 */
	/* ---- */
	subq	v0, 1, v0		/* count-- */
	stq	t3, (3*8)(a1)		/* store 3 */
	stq	t4, (4*8)(a1)		/* store 4 */
	stq	t5, (5*8)(a1)		/* store 5 */
	/* ---- */
	stq	t6, (6*8)(a1)		/* store 6 */
	stq	t7, (7*8)(a1)		/* store 7 */
	addq	a1, (8*8), a1		/* a1 = a1 + 8-quads */
	bne	v0, 1b			/* loop around if count != 0 */
	/* ---- */

	RET
	END(pmap_copy_page)
