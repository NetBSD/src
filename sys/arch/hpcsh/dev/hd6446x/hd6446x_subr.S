/*	$NetBSD: hd6446x_subr.S,v 1.2 2002/05/09 12:41:09 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sh3/asm.h>
#include <sh3/locore.h>
#include <hpcsh/dev/hd6446x/hd6446xintcreg.h>

/*
 * int hd6446x_intr_raise(int s):
 *	raise SR.IMASK and HD6446x ICU interrupt mask to 's'.
 *	if current SR.IMASK is greater equal 's',nothing to do.
 *	Returns previous SR.IMASK.
 */
NENTRY(hd6446x_intr_raise)
	stc	sr,	r2
	mov	#0x78,	r1
	mov	r2,	r0
	shll	r1		/* r1 = 0xf0 */
	and	r1,	r0	/* r0 = SR & 0xf0 */
	cmp/ge	r4,	r0	/* r0 >= r4 ? T = 1 */
	bt/s	1f
	 not	r1,	r1	/* r1 = 0xffffff0f */
	and	r1,	r2	/* r2 = SR & ~0xf0 */
	mov	r4,	r1
	or	r2,	r1	/* r1 = (SR & ~0xf0) | s */
	ldc	r1,	sr	/* SR = r1 */
	shlr2	r4
	shlr	r4
	mov.l	2f,	r1
	add	r4,	r1
	mov.w	@r1,	r2	/* r2 = hd6446x_imask[s >> 4] */
	mov.l	3f,	r1
	mov.w	r2,	@r1	/* Set new interrupt mask to HD6446x */
1:	rts
	 nop	/* return (SR & 0xf0) */
	.align	2
2:	.long	_C_LABEL(hd6446x_imask)
3:	.long	HD6446X_NIMR

/*
 * int hd6446x_intr_resume(int s):
 *	Set SR.IMASK and HD6446x interrupt mask register to
 *	's' interrupt level. Returns previous SR.IMASK.
 */
NENTRY(hd6446x_intr_resume)
	mov	r4,	r0
	shlr2	r0
	shlr	r0
	mov.l	2f,	r1
	add	r0,	r1
	mov.w	@r1,	r2	/* r2 = hd6446x_imask[s >> 4] */
	mov.l	3f,	r1
	mov.w	r2,	@r1	/* Set new interrupt mask to HD6446x */
	stc	sr,	r0	/* r0 = SR */
	mov	#0x78,	r2
	shll	r2		/* r2 = 0x000000f0 */
	not	r2,	r1	/* r1 = 0xffffff0f */
	and	r0,	r1	/* r1 = (SR & ~0xf0) */
	or	r1,	r4	/* r4 = (SR & ~0xf0) | level */
	ldc	r4,	sr	/* Set new IMASK to SR */
	rts
	 and	r2,	r0	/* return (SR & 0xf0) */
	.align	2
2:	.long	_C_LABEL(hd6446x_imask)
3:	.long	HD6446X_NIMR
