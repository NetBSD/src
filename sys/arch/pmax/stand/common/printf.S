/*	$NetBSD: printf.S,v 1.6 2009/12/14 00:46:11 matt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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
#include <machine/dec_prom.h>

LEAF(printf)
	lw	v0, _C_LABEL(callv)	# get pointer to call back vectors
	lw	t9, 0x30(v0)	# offset for callv->_printf
	nop
#ifdef __mips_n32
	/*
	 * If someone calls printf with more than 4 args on n32,
	 * we need to synthesize a o32 call.  For now we assume no more than
	 * 8 arguments and no 64bit values.
	 */
	subu	sp, sp,	48
	sw	a4, 16(sp)
	sw	a5, 20(sp)
	sw	a6, 24(sp)
	sw	a7, 28(sp)
	sw	ra, 44(sp)
	jalr	t9		# call PROM printf
	 nop
	lw	ra, 44(sp)
	j	ra
	 addu	sp, sp, 48
#else
	j	t9		# call PROM printf
	nop
#endif
END(printf)
