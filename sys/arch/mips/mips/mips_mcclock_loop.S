/* $NetBSD: mips_mcclock_loop.S,v 1.4 2009/12/14 00:46:06 matt Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	noreorder

/*
 * Provide assembly versions of the clock timing calculation loops that
 * aren't effected by changes in compiler optimisation.
 *
 * These two functions are not profiled!
 */


LEAF_NOPROFILE(mips_mcclock_loop_with_clock)
	PTR_SUBU	sp, CALLFRAME_SIZ
	REG_S		s0, CALLFRAME_S0(sp)
	REG_S		ra, CALLFRAME_RA(sp)
	j		2f
	 move	s0, zero		# iters = 0;
1:
	.set	push
	.set	mips32
	ssnop				# asm ("ssnop;ssnop;ssnop;ssnop");
	ssnop
	ssnop
	ssnop
	.set	pop

	addu		s0, 1			# iters++;
2:	jal		mips_cp0_cause_read	# v0 = mips_cp0_cause_read();
	 nop
	and		v0, a0			# v0 &= clockmask;
	beqz		v0, 1b			# if zero then repeat
	 move		v0, s0			# return iters
	REG_L		ra, CALLFRAME_RA(sp)
	REG_S		s0, CALLFRAME_S0(sp)
	j		ra
	 PTR_ADDU	sp, CALLFRAME_SIZ
END(mips_mcclock_loop_with_clock)

LEAF_NOPROFILE(mips_mcclock_loop_without_clock)
	PTR_SUBU	sp, CALLFRAME_SIZ
	REG_S		s0, CALLFRAME_S0(sp)
	REG_S		ra, CALLFRAME_RA(sp)
	j		2f
	 move		s0, zero		# iters = 0;
1:
	.set	push
	.set	mips32
	ssnop				# asm ("ssnop;ssnop");
	ssnop
	.set	pop

	addu		s0, 1			# iters++;
2:	jal		mips_cp0_cause_read	# v0 = mips_cp0_cause_read();
	 nop
	and		v0, a0			# v0 &= clockmask;
	beqz		v0, 1b			# if zero then repeat
	 move		v0, s0			# return iters
	REG_L		ra, CALLFRAME_RA(sp)
	REG_L		s0, CALLFRAME_S0(sp)
	j		ra
	 PTR_ADDU	sp, CALLFRAME_SIZ
END(mips_mcclock_loop_without_clock)
