/*	$NetBSD: locore.S,v 1.1 1999/07/08 11:48:05 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	noreorder
	.text
	.align	2

	.globl	_start
_start:
	j	boot
	nop

/* void flushicache(addr, len); */

	.globl	flushicache
flushicache:
	mfc0	v0, MIPS_COP_0_STATUS		# save SR
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	la	v1, 1f
	or	v1, MIPS_KSEG1_START		# run uncached
	j	v1
	nop
1:
	li	v1, MIPS_SR_ISOL_CACHES | MIPS_SR_SWAP_CACHES
	mtc0	v1, MIPS_COP_0_STATUS
	nop
	addu	a1, a1, a0			# compute ending address
2:
	sb	zero, -4(a0)
	bne	a0, a1, 2b
	addu	a0, a0, 4

	mtc0	v0, MIPS_COP_0_STATUS		# enable interrupts
	j	ra				# return and run cached
	nop
