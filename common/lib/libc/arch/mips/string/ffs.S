/*	$NetBSD: ffs.S,v 1.1.40.3 2010/04/22 17:50:30 matt Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <mips/asm.h>

RCSID("$NetBSD: ffs.S,v 1.1.40.3 2010/04/22 17:50:30 matt Exp $")

/* bit = ffs(value) */

	.set	noreorder

#if __mips == 64 || __mips == 32
LEAF(ffs)
#ifndef _LP64
XLEAF(ffsl)
#endif
	.set	push
	.set	mips32
	li	v1, 32
#if __mips == 64
	sll	a0, a0, 0
#endif
	negu	a1, a0
	and	a0, a1
	clz	v0, a0
	j	ra
	 subu	v0, v1, v0
	.set	pop
END(ffs)
#if defined(_LP64) && __mips == 64
LEAF(ffsl)
	li	v1, 64
	negu	a1, a0
	and	a0, a1
	dclz	v0, a0
	j	ra
	 subu	v0, v1, v0
END(ffsl)
#endif
#else /* __mips != 64 && __mips != 32 */

#ifdef _LP64
XLEAF(ffsl)
	beqz	a0, 6f			# fast escape if 0
	 li	v0, 0

	li	v0, 1
#if 1
	li	a3, 0xffffffff		# initial mask
	b	1f
	 li	a2, 32			# bit count of mask
#else
	dsll	v1, a0, 32		# discard the upper 32 bits
	bnez	v1, 1f			# any of the lower 32 bits set?
	 nop
	addiu	v0, 32			# nope, then bit must be in the upper 32
	b	1f
	 dsrl	a0, 32			# discard low 32 bits
#endif
#endif /* _LP64 */
LEAF(ffs)
#ifndef _LP64
XLEAF(ffsl)
#endif /* !_LP64 */
	beqz	a0, 6f
	 li	v0, 0

	li	v0, 1
	li	a3, 0xffff		# initial mask
	li	a2, 16			# bit count of mask
1:
#if 1
	and	v1, a0, a3		# focus no lower half of bits left
	bnez	v1, 2f			# any of the lower half set?
	 nop
	addu	v0, a2			# nope, then bit is in the upper half
#ifdef _LP64
	dsrlv	a0, a0, a2		# discard low bits
#else
	srlv	a0, a0, a2		# discard low bits
#endif
2:
	srl	a2, 1			# divide bit count by 2
	bnez	a2, 1b			# still bits left to text?
	 srlv	a3, a3, a2		# shrink mask in half
#else
	andi	v1, a0, 0xffff		# narrow focus to low 16 bits of 32 left
	bnez	v1, 2f			# any of the lower 16 bits set?
	 nop
	addiu	v0, 16			# nope, then bit must be in the upper 16
	srl	a0, 16			# discard low 16 bits
2:
	andi	v1, a0, 0xff		# narrow focus to low 8 bits of 16 left
	bnez	v1, 3f			# any of the lower 8 bits set?
	 nop
	addiu	v0, 8			# nope, then bit must be in the upper 8
	srl	a0, 8			# discard low 8 bits
3:
	andi	v1, a0, 0xf		# narrow focus to low 4 bits of 8 left
	bnez	v1, 4f			# any of the lower 4 bits set?
	 nop
	addiu	v0, 4			# nope, then bit must be in the upper 4
	srl	a0, 4			# discard low 4 bits
4:
	andi	v1, a0, 0x3		# narrow focus to low 2 bits of 4 left
	bnez	v1, 5f			# any of the lower 2 bits set?
	 nop
	addiu	v0, 2			# nope, then bit must be in the upper 2
	srl	a0, 2			# discard low 2 bits
5:
	andi	v1, a0, 0x1
	bnez	v1, 6f
	 nop
	addiu	v0, 1			# nop, then first must be in bit 1
#endif
6:
	j	ra
	 nop
END(ffs)
#endif /* __mips == 64 || __mips == 32 */
