/*	$NetBSD: 4xx_locore.S,v 1.6 2006/10/07 14:44:22 tsutsui Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is not a standalone file. To use it, #inlcude it at
 * the end of your port's locore.S
 */

/*
 * Pull in common PowerPC locore subroutines.
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Pull in common trap vector code.
 */
#include <powerpc/ibm4xx/trap_subr.S>
#include <powerpc/ibm4xx/4xx_trap_subr.S>

/*
 * Include PIO routines
 */
#include <powerpc/powerpc/pio_subr.S>

	.globl	_C_LABEL(ppc4xx_reset)
_C_LABEL(ppc4xx_reset):
	mfspr	%r3,SPR_DBCR0
	oris	%r3,%r3,DBCR0_RST_SYSTEM@h
	mtspr	SPR_DBCR0,%r3
	ba	0

#if 0
/*
 * XXXX the following doesn't quite work right yet.
 */
/*
 * void bcopy(const void *src, void *dst, size_t len);
 *
 * swap %r3 and %r4 and fall through to memcopy.
 */
	.globl _C_LABEL(bcopy)
_C_LABEL(bcopy):
	mr	%r0,%r3
	mr	%r3,%r4
	mr	%r4,%r0
	/* FALLTHROUGH */

/*
 * void * memcpy(void *dst (%r3), const void *src (%r4), size_t len (%r5));
 *
 * Copy memory (obviously)
 *
 * We will try to do data cache block aligned stores so we
 * can use block allocate and not have to read from the
 * destination.
 *
 * Register use:
 *
 *	%r1		stack (of course)
 *	%r3		dst
 *	%r4		src
 *	%r5		len
 *	%r6		tmp
 *	%r7		holds 32
 *	%r8		holds dst
 *	%r24-%r31	block move regs
 *
 */

ENTRY(memcpy)
	stwu	%r1,-(10*4)(%r1)	/* Allocate some RAM to save 8 regs to. */
	cmpwi	%r5, 32		/* Less than 32 bytes ? */
	stmw	%r24,8(%r1)	/* Save ALL regs (could be optimized) */

	mr	%r8,%r3		/* save dst */
	li	%r7,32

	dcbt	%r0,%r4		/* Start bringing in cache line. */
	blt	1f		/* Finish up */

	neg	%r6,%r3		/* Find how far unaligned we are... */
	andi.	%r6,%r6,31	/* Cache-align dest. */
	mtxer	%r6
	sub	%r5,%r5,%r6	/* subtract count */
	lswx	%r24,%r0,%r4	/* Load some. */
	add	%r4,%r4,%r6
	dcbt	%r0,%r4		/* Fetch next line */
	stswx	%r24,%r0,%r3	/* Store some */
	add	%r3,%r3,%r6
	addic.	%r6,%r5,-32	/* Pre-decrement next line */
	ble	1f		/* Less than 32-bytes? finishup */

	/* Dest should not be cache line aligned. */
	/* XXX need gas 2.11 to grok dcba insn */
#ifdef GAS_2_11
	dcba	%r0,%r3		/* Allocate a line */
#else
	.long	0x7c001dec	/* dcba	0,r3 */
#endif
0:
	dcbt	%r7,%r4		/* Bring in the next line, too */

	lswi	%r24,%r4,32
	addi	%r4,%r4,32	/* Inc src */
	mr	%r5,%r6

	addic.	%r6,%r5,-32
	stswi	%r24,%r3,32
	addi	%r3,%r3,32	/* Inc dst */
#ifdef GAS_2_11
	dcba	0,%r3		/* Allocate another line */
#else
	.long	0x7c071dec	/* dcba	%r7,%r3 */
#endif	
	bgt	0b
1:
	mtxer	%r5		/* Store byte count */
	lswx	%r24,%r0,%r4	/* Load up to 32 bytes */
	stswx	%r24,%r0,%r3	/* Store up to 32 bytes */

	mr	%r3,%r8		/* Return dst */

	lmw	%r24,8(%r1)
	addi	%r1,%r1,(10*4)
	blr
#endif
