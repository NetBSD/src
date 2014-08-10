/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>
#include "assym.h"

RCSID("$NetBSD: spe_subr.S,v 1.4 2014/08/10 18:08:33 joerg Exp $")

	.text
#ifndef __clang__
	.machine	e500x2
#endif
/*
 * Lintstub: void vec_load_from_vreg(const struct vreg *);
 */
ENTRY(vec_load_from_vreg)
	/*
	 * Load and initialize the accumulator.  Don't care about trashing
	 * r0's high half since we are about to load it.
	 */
	evldd %r0,(8 << 4)(%r3)
	evmra %r0,%r0

	/*
	 * SPEFSCR is in the trapframe.
	 */

	/*
	 * Since the high part of the register is saved in vreg (because we
	 * are loading it), and we know SPE is on (to load it), just load two
	 * high parts at one time and merge appropriately.  For each even-odd
	 * register pair, the evldw will load high part for the even register
	 * in the high 32 bits and the high part for the odd regiter in the
	 * low 32 bits.
	 *
	 * The evmergelo will move the lower half of r0 to the high half of the
	 * destination register and evmergehilo will merge the high half of r0
	 * and lower half of the other register.
	 */
	evldw %r0,(30 << 2)(%r3)
		evmergelo %r31,%r0,%r31; evmergehilo %r30,%r0,%r30
	evldw %r0,(28 << 2)(%r3)
		evmergelo %r29,%r0,%r29; evmergehilo %r28,%r0,%r28
	evldw %r0,(26 << 2)(%r3)
		evmergelo %r27,%r0,%r27; evmergehilo %r26,%r0,%r26
	evldw %r0,(24 << 2)(%r3)
		evmergelo %r25,%r0,%r25; evmergehilo %r24,%r0,%r24
	evldw %r0,(22 << 2)(%r3)
		evmergelo %r23,%r0,%r23; evmergehilo %r22,%r0,%r22
	evldw %r0,(20 << 2)(%r3)
		evmergelo %r21,%r0,%r21; evmergehilo %r20,%r0,%r20
	evldw %r0,(18 << 2)(%r3)
		evmergelo %r19,%r0,%r19; evmergehilo %r18,%r0,%r18
	evldw %r0,(16 << 2)(%r3)
		evmergelo %r17,%r0,%r17; evmergehilo %r16,%r0,%r16
	evldw %r0,(14 << 2)(%r3)
		evmergelo %r15,%r0,%r15; evmergehilo %r14,%r0,%r14
	evldw %r0,(12 << 2)(%r3)
		evmergelo %r13,%r0,%r13; evmergehilo %r12,%r0,%r12
	/*
	 * Done with callee-saved registers.  For caller-saved we can just
	 * trash the register contents.  So load the two words with the high
	 * halves into the appropriate register and merge the half for the
	 * next register into it.
	 */
	evldw %r10,(10 << 2)(%r3)
		evmergelo %r11,%r10,%r11
	evldw %r8,(8 << 2)(%r3)
		evmergelo %r9,%r8,%r9
	evldw %r6,(6 << 2)(%r3)
		evmergelo %r7,%r6,%r7
	evldw %r4,(4 << 2)(%r3)
		evmergelo %r5,%r4,%r5

	/*
	 * R2 isn't a callee-saved, so load into r0 because we still need r3
	 */
	evldw %r0,(2 << 2)(%r3)
		evmergelo %r3,%r0,%r3; evmergehilo %r2,%r0,%r2
	evldd %r0,(0 << 2)(%r3)
		evmergelo %r1,%r0,%r1		/* why bother? */

	blr
END(vec_load_from_vreg)

/*
 * Lintstub: void vec_unload_to_vreg(struct vreg *);
 */
ENTRY(vec_unload_to_vreg)
	evmergehi %r4,%r4,%r0;	/* save r0's high part in lo r4 */
	evmergehi %r0,%r0,%r1; evstdw %r0,(0 << 2)(%r3)
	evmergehi %r0,%r2,%r3; evstdw %r0,(2 << 2)(%r3)
	evmergehi %r0,%r4,%r5; evstdw %r0,(4 << 2)(%r3)
	evmergehi %r0,%r6,%r7; evstdw %r0,(6 << 2)(%r3)
	evmergehi %r0,%r8,%r9; evstdw %r0,(8 << 2)(%r3)
	evmergehi %r0,%r10,%r11; evstdw %r0,(10 << 2)(%r3)
	evmergehi %r0,%r12,%r13; evstdw %r0,(12 << 2)(%r3)
	evmergehi %r0,%r14,%r15; evstdw %r0,(14 << 2)(%r3)
	evmergehi %r0,%r16,%r17; evstdw %r0,(16 << 2)(%r3)
	evmergehi %r0,%r18,%r19; evstdw %r0,(18 << 2)(%r3)
	evmergehi %r0,%r20,%r21; evstdw %r0,(20 << 2)(%r3)
	evmergehi %r0,%r22,%r23; evstdw %r0,(22 << 2)(%r3)
	evmergehi %r0,%r24,%r25; evstdw %r0,(24 << 2)(%r3)
	evmergehi %r0,%r26,%r27; evstdw %r0,(26 << 2)(%r3)
	evmergehi %r0,%r28,%r29; evstdw %r0,(28 << 2)(%r3)
	evmergehi %r0,%r30,%r31; evstdw %r0,(30 << 2)(%r3)

	/*
	 * Now save the accumulator.
	 */
	evxor %r0,%r0,%r0		/* zero r0 */
	evaddumiaaw %r0,%r0		/* r0 = accum + r0 */
	evstdd %r0,(8 << 4)(%r3)	/* store it */

	evmergelo %r0,%r4,%r0		/* retore r0's high half */

	/*
	 * The SPEFSCR will be restored when the exception returns.
	 */
	blr
END(vec_unload_to_vreg)
