/*	$NetBSD: convert_xmm_s87.c,v 1.3.10.2 2014/08/20 00:03:29 tls Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2001, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: convert_xmm_s87.c,v 1.3.10.2 2014/08/20 00:03:29 tls Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <x86/fpu.h>

void
process_xmm_to_s87(const struct fxsave *sxmm, struct save87 *s87)
{
	unsigned int tag, ab_tag;
	const struct fpaccfx *fx_reg;
	struct fpacc87 *s87_reg;
	int i;

	/*
	 * For historic reasons core dumps and ptrace all use the old save87
	 * layout.  Convert the important parts.
	 * getucontext gets what we give it.
	 * setucontext should return something given by getucontext, but
	 * we are (at the moment) willing to change it.
	 *
	 * It really isn't worth setting the 'tag' bits to 01 (zero) or
	 * 10 (NaN etc) since the processor will set any internal bits
	 * correctly when the value is loaded (the 287 believed them).
	 *
	 * Additionally the s87_tw and s87_tw are 'indexed' by the actual
	 * register numbers, whereas the registers themselves have ST(0)
	 * first. Pairing the values and tags can only be done with
	 * reference to the 'top of stack'.
	 *
	 * If any x87 registers are used, they will typically be from
	 * r7 downwards - so the high bits of the tag register indicate
	 * used registers. The conversions are not optimised for this.
	 *
	 * The ABI we use requires the FP stack to be empty on every
	 * function call. I think this means that the stack isn't expected
	 * to overflow - overflow doesn't drop a core in my testing.
	 *
	 * Note that this code writes to all of the 's87' structure that
	 * actually gets written to userspace.
	 */

	/* FPU control/status */
	s87->s87_cw = sxmm->fx_cw;
	s87->s87_sw = sxmm->fx_sw;
	/* tag word handled below */
	s87->s87_ip = sxmm->fx_ip;
	s87->s87_opcode = sxmm->fx_opcode;
	s87->s87_dp = sxmm->fx_dp;

	/* FP registers (in stack order) */
	fx_reg = sxmm->fx_87_ac;
	s87_reg = s87->s87_ac;
	for (i = 0; i < 8; fx_reg++, s87_reg++, i++)
		*s87_reg = fx_reg->r;

	/* Tag word and registers. */
	ab_tag = sxmm->fx_tw & 0xff;	/* Bits set if valid */
	if (ab_tag == 0) {
		/* none used */
		s87->s87_tw = 0xffff;
		return;
	}

	tag = 0;
	/* Separate bits of abridged tag word with zeros */
	for (i = 0x80; i != 0; tag <<= 1, i >>= 1)
		tag |= ab_tag & i;
	/* Replicate and invert so that 0 => 0b11 and 1 => 0b00 */
	s87->s87_tw = (tag | tag >> 1) ^ 0xffff;
}

void
process_s87_to_xmm(const struct save87 *s87, struct fxsave *sxmm)
{
	unsigned int tag, ab_tag;
	struct fpaccfx *fx_reg;
	const struct fpacc87 *s87_reg;
	int i;

	/*
	 * ptrace gives us registers in the save87 format and
	 * we must convert them to the correct format.
	 *
	 * This code is normally used when overwriting the processes
	 * registers (in the pcb), so it musn't change any other fields.
	 *
	 * There is a lot of pad in 'struct fxsave', if the destination
	 * is written to userspace, it must be zeroed first.
	 */

	/* FPU control/status */
	sxmm->fx_cw = s87->s87_cw;
	sxmm->fx_sw = s87->s87_sw;
	/* tag word handled below */
	sxmm->fx_ip = s87->s87_ip;
	sxmm->fx_opcode = s87->s87_opcode;
	sxmm->fx_dp = s87->s87_dp;

	/* Tag word */
	tag = s87->s87_tw;	/* 0b11 => unused */
	if (tag == 0xffff) {
		/* All unused - values don't matter, zero for safety */
		sxmm->fx_tw = 0;
		memset(&sxmm->fx_87_ac, 0, sizeof sxmm->fx_87_ac);
		return;
	}

	tag ^= 0xffff;		/* So 0b00 is unused */
	tag |= tag >> 1;	/* Look at even bits */
	ab_tag = 0;
	i = 1;
	do
		ab_tag |= tag & i;
	while ((tag >>= 1) >= (i <<= 1));
	sxmm->fx_tw = ab_tag;

	/* FP registers (in stack order) */
	fx_reg = sxmm->fx_87_ac;
	s87_reg = s87->s87_ac;
	for (i = 0; i < 8; fx_reg++, s87_reg++, i++)
		fx_reg->r = *s87_reg;
}
