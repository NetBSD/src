/* $NetBSD: atomic.s,v 1.7 1999/11/28 19:47:13 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

__KERNEL_RCSID(4, "$NetBSD: atomic.s,v 1.7 1999/11/28 19:47:13 thorpej Exp $")

/*
 * Misc. `atomic' operations.
 */

	.text
inc4:	.stabs	__FILE__,132,0,0,inc4; .loc	1 __LINE__

/*
 * alpha_atomic_testset_l:
 *
 *	Atomically test and set a single bit in a longword (32-bit).
 *
 * Inputs:
 *
 *	a0	Address of longword in which to perform the t&s.
 *	a1	Mask of bit (just one!) to test and set.
 *
 * Outputs:
 *
 *	v0	0 if bit already set, non-0 if bit set successfully.
 */
	.text
LEAF(alpha_atomic_testset_l,2)
1:	ldl_l	t0, 0(a0)
	and	t0, a1, t3
	bne	t3, 2f		/* already set, return(0) */
	or	t0, a1, v0
	stl_c	v0, 0(a0)
	beq	v0, 3f		/* branch-prediction: "not taken" */
	mb
	RET			/* v0 != 0 */
2:	mov	zero, v0
	RET
3:	br	1b		/* forward branch to here */
	END(alpha_atomic_testset_l)

/*
 * alpha_atomic_setbits_q:
 *
 *	Atomically set bits in a quadword.
 *
 * Inputs:
 *
 *	a0	Address of quadword in which to set the bits.
 *	a1	Mask of bits to set.
 *
 * Outputs:
 *
 *	None.
 */
	.text
LEAF(alpha_atomic_setbits_q,2)
1:	ldq_l	t0, 0(a0)
	or	t0, a1, t0
	stq_c	t0, 0(a0)
	beq	t0, 2f
	mb
	RET
2:	br	1b
	END(alpha_atomic_setbits_q)

/*
 * alpha_atomic_clearbits_q:
 *
 *	Atomically clear bits in a quadword.
 *
 * Inputs:
 *
 *	a0	Address of quadword in which to clear the bits.
 *	a1	Mask of bits to clear.
 *
 * Outputs:
 *
 *	None.
 */
	.text
LEAF(alpha_atomic_clearbits_q,2)
	ornot	zero, a1, t1
1:	ldq_l	t0, 0(a0)
	and	t0, t1, t0
	stq_c	t0, 0(a0)
	beq	t0, 2f
	mb
	RET
2:	br	1b
	END(alpha_atomic_setbits_q)

/*
 * alpha_atomic_testset_q:
 *
 *	Atomically test and set a single bit.
 *
 * Inputs:
 *
 *	a0	Address of quadword in which to perform the t&s.
 *	a1	Mask of bit (just one!) to test and set.
 *
 * Outputs:
 *
 *	v0	0 if bit already set, non-0 if bit set successfully.
 */
	.text
LEAF(alpha_atomic_testset_q,2)
1:	ldq_l	t0, 0(a0)
	and	t0, a1, t3
	bne	t3, 2f	/* Already set, return(0) */
	or	t0, a1, v0
	stq_c	v0, 0(a0)
	beq	v0, 3f
	mb
	RET				/* v0 != 0 */
2:	mov	zero, v0
	RET
3:	br	1b
	END(alpha_atomic_testset_q)

/*
 * alpha_atomic_loadlatch_q:
 *
 *	Atomically load and latch a quadword value.
 *
 * Inputs:
 *
 *	a0	Address of quadword to load and latch.
 *	a1	Value to store at 0(a0) once value has been loaded.
 *
 * Outputs:
 *
 *	v0	Value loaded from 0(a0).
 */
	.text
LEAF(alpha_atomic_loadlatch_q,2)
1:	mov	a1, t0
	ldq_l	v0, 0(a0)
	stq_c	t0, 0(a0)
	beq	t0, 2f
	mb
	RET
2:	br	1b
	END(alpha_atomic_loadlatch_q)

/*
 * alpha_atomic_add_q:
 *
 *	Atomically add a value to a quadword.
 *
 * Inputs:
 *
 *	a0	Address of quadword to add to.
 *	a1	Value to add to quadword.
 *
 * Outputs:
 *
 *	None.
 */
	.text
LEAF(alpha_atomic_add_q,2)
1:	ldq_l	t0, 0(a0)
	addq	t0, a1, t0
	stq_c	t0, 0(a0)
	beq	t0, 2f
	mb
	RET
2:	br	1b
	END(alpha_atomic_add_q)

/*
 * alpha_atomic_sub_q:
 *
 *	Atomically subtract a value from a quadword.
 *
 * Inputs:
 *
 *	a0	Address of quadword to subtract from.
 *	a1	Value to subtract from quadword.
 *
 * Outputs:
 *
 *	None.
 */
	.text
LEAF(alpha_atomic_sub_q,2)
1:	ldq_l	t0, 0(a0)
	subq	t0, a1, t0
	stq_c	t0, 0(a0)
	beq	t0, 2f
	mb
	RET
2:	br	1b
	END(alpha_atomic_sub_q)
