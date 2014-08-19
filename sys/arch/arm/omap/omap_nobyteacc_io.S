/*	$NetBSD: omap_nobyteacc_io.S,v 1.2.2.1 2014/08/20 00:02:47 tls Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Byte write bus space function for 8-bit IO registers mapped on a
 * 16-bit bus with an A0 address line.
 */

#include <machine/asm.h>
#include <arm/armreg.h>

ENTRY(nobyteacc_bs_w_1)
	tst	r2, #1			/* even or odd byte offset? */
#ifndef _ARM_ARCH_7
	andeq	r3, r3, #0x00ff		/* mask out junk in hi bytes */
	lslne	r3, r3, #8		/* odd: or data shifted to hi byte */
#endif
	bic	r2, r2, #1		/* halfword-align offset */
	add	r1, r1, r2		/* halfword address */
	mrs	r2, cpsr		/* r2 = old interrupt state */
#ifdef _ARM_ARCH_6
	cpsid	if			/* interrupts off */
#else
	orr	r0, r2, #(I32_bit | F32_bit) /* interrupts off */
	msr	cpsr, r0
#endif
	ldrh	r0, [r1]		/* read the halfword */
#ifdef __thumb__
	ite	eq
#endif
#ifdef _ARM_ARCH_7
	bfieq	r0, r3, #0, #8		/* even: insert new data in lo byte */
	bfine	r0, r3, #8, #8		/* even: insert new data in hi byte */
#else
	biceq	r0, r0, #0x00ff		/* even: preserve existing hi byte */
	andne	r0, r0, #0x00ff		/* odd:	preserve existing lo byte */
	orr	r0, r0, r3		/* or in data */
#endif
	strh	r0, [r1]		/* store back halfword */
	msr	cpsr, r2		/* restore interrupts */
	RET
END(nobyteacc_bs_w_1)
