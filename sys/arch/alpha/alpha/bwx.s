/* $NetBSD: bwx.s,v 1.3 1997/11/03 04:22:00 ross Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(3, "$NetBSD: bwx.s,v 1.3 1997/11/03 04:22:00 ross Exp $")

/*
 * Alpha Byte/Word Extension instructions.  These are functions because
 * gas(1) cannot currently switch targets on the fly.  Eventually, these
 * might be replaced by inline __asm() functions.
 *
 * These instructions are available on EV56 (21164A) and later processors.
 *
 * See "Alpha Architecture Handbook, Version 3", DEC order number EC-QD2KB-TE.
 */

/*
 * XXX These are .long'd because we pull this into locore.s, which
 * XXX is assembled with only gas(1)'s "base alpha" target.
 */
	.text
inc1:	.stabs	__FILE__,132,0,0,inc1; .loc	1 __LINE__
LEAF(alpha_ldbu,1)
	.long	0x28100000	/* ldbu v0, 0(a0) */
	RET
	END(alpha_ldbu)

	.text
LEAF(alpha_ldwu,1)
	.long	0x30100000	/* ldwu v0, 0(a0)  */
	RET
	END(alpha_ldwu)

	.text
LEAF(alpha_stb,1)
	.long	0x3a300000	/* stb a1, 0(a0) */
	RET
	END(alpha_stb)

	.text
LEAF(alpha_stw,1)
	.long	0x36300000	/* stw a1, 0(a0) */
	RET
	END(alpha_stw)

	.text
LEAF(alpha_sextb,1)
	.long	0x73f00000	/* sextb a0, v0 */
	RET
	END(alpha_sextb)

	.text
LEAF(alpha_sextw,1)
	.long	0x73f00020	/* sextw a0, v0 */
	RET
	END(alpha_sextw)
