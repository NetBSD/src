/*	$NetBSD: locore_subr.S,v 1.1 2002/02/19 17:21:18 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <machine/asm.h>
	
	.text
	.align 5	/* align cache line size (32B) */

/*
 * u_int32_t _cpu_intr_suspend(void):
 *	Mask all external interrupt. returns previous mask. (SR.IMASK)
 */					
NENTRY(_cpu_intr_suspend)
	stc	sr, r0		/* r0 = SR */	
	mov	#0x78, r1
	shll	r1		/* r1 = 0x000000f0 */
	mov	r0, r2		/* r2 = SR */
	or	r1, r2		/* r2 |= 0x000000f0 */
	ldc	r2, sr		/* SR = r2 */
	rts
	and	r1, r0		/* r0 = SR & 0x000000f0 */

/*
 * void _cpu_intr_resume(u_int32_t s):
 *	restore 's' interrupt mask. (SR.IMASK)
 */
NENTRY(_cpu_intr_resume)
	stc	sr, r0		/* r0 = SR */	
	mov	#0x78, r1
	shll	r1		/* r1 = 0x000000f0 */
	not	r1, r1		/* r1 = 0xffffff0f */
	and	r1, r0		/* r0 &= ~0x000000f0 */
	or	r4, r0		/* r0 |= old SR.IMASK */
	ldc	r0, sr		/* SR = r0 (don't move to delay slot) */
	rts
	nop	

/*	
 * u_int32_t _cpu_exception_suspend(void):
 *	Block exception (SR.BL). if external interrupt raise, pending interrupt.
 *	if exception occur, jump to 0xa0000000 (hard reset).
 */	
NENTRY(_cpu_exception_suspend)	
	stc	sr, r0		/* r0 = SR */
	mov	#0x10, r1
	swap.b	r1, r1
	mov	r0, r2		/* r2 = r0 */
	swap.w	r1, r1		/* r1 = 0x10000000 */
	or	r1, r2		/* r2 |= 0x10000000 */
	ldc	r2, sr		/* SR = r2 */
	rts
	and	r1, r0		/* r0 &= 0x10000000 */

/*
 * void _cpu_exception_resume(u_int32_t s):
 *	restore 's' exception mask. (SR.BL)
 */
NENTRY(_cpu_exception_resume)	
	stc	sr, r0		/* r0 = SR */	
	mov	#0x10, r1
	swap.b	r1, r1
	swap.w	r1, r1
	not	r1, r1		/* r1 = ~0x10000000 */
	and	r1, r0		/* r0 &= ~0x10000000 */
	or	r4, r0		/* r0 |= old SR.BL */
	ldc	r0, sr		/* SR = r0 (don't move to delay slot) */	
	rts
	nop
