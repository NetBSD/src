/*	$NetBSD: lmc1982.S,v 1.1.4.2 2001/10/05 22:27:42 reinoud Exp $	*/

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
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
 *      This product includes software developed by Michael L. Hitch.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * NS LMC1982, Stereo and volume control.
 */


#include <machine/asm.h>
#include <arm/iomd/iomdreg.h>
#include <machine/mainbus/lmc1982.h>

/*
 * r0=address
 * r1=value
 * r2 control IOCR
 * r3=control FREQCON
 * r4 timeout
 * r5 FREQimage
 * r6 FREQCON
 * r7 IOCBase (IOCR address)
 * r4, r8 scratch
 */

ENTRY(volume_ctl)
	stmfd	sp!, {r0-r8, lr}

	/*
	 * operate the volume control circuit
	 */
	mov	r7, #(IOMD_BASE)
	add	r6, r7, #(FREQCON << 2)	/* location of FREQCON register */
	ldr	r5, Lvfreqcon		/* frequency reg shadow */
	orr	r0,r0,#0x40		/* chip address adder */

	ldrb	r2,[r7]			/* get IOCR value */
	orr	r2,r2,#IOCR_AUDIO	/*insure data is high*/
	strb	r2,[r7]

	mov	r4,#8
	ldrb	r3,[r5]			/* get freqcon */
	orr	r3,r3,#(LID | LCLOCK)	/* ID and CLOCK high */
	strb	r3,[r6]
	mov	r4,#10
	bl	_waitabit		/* wait 10 usec */
	bic	r3,r3,#(LID)
	strb	r3,[r6]			/* lower ID bit */

	bl	_shift

	orr	r3,r3,#(LID | LCLOCK)	/* ID and CLOCK high */
	strb	r3,[r6]
	mov	r0,r1
	bl	_shift
	
	mov	r4,#10
	bl	_waitabit		/* wait 10 usec */

	bic	r3,r3,#(LID)
	strb	r3,[r6]			/* lower ID bit */

	mov	r4,#1
	bl	_waitabit		/* wait 1 usec */

	orr	r3,r3,#(LID | LCLOCK)	/* ID and CLOCK high */
	strb	r3,[r6]

	ldmfd	sp!, {r0-r8, pc}

Lvfreqcon:
	.word	_vfreqcon

_shift:	/* data in r0 */
	mov	r4,#8			/* number of bits in address */
sloop:
	bic	r3,r3,#(LCLOCK)
	strb	r3,[r6]			/* lower CLOCK bit */

	movs	r0,r0,LSR #1		/* get bit */
	orrcs	r2,r2,#IOCR_AUDIO	/* insure data is high */
	biccc	r2,r2,#IOCR_AUDIO	/* insure data is low */
	strb	r2,[r7]

	orr	r3,r3,#(LCLOCK)
	strb	r3,[r6]			/* raise CLOCK bit */

	subs	r4,r4,#1
	bne	sloop
	mov	pc,lr

_waitabit:
	mov	r8,#15
wloop:
	subs	r8,r8,#1
	bne	wloop
	subs	r4,r4,#1
	bne	_waitabit
	mov	pc,lr

	/*
	 * r0: addr
	 * r1: counter
	 */
ENTRY(conv_jap)
	stmfd	sp!, {r2-r4, lr}
	movs	r2, r1		/* counter */
	beq	conv_jap_end
cloop:
	ldrb	r3, [r0]
	eor	r3, r3, #0x80
	strb	r3, [r0], #1
	subs	r2, r2, #1
	bne	cloop

conv_jap_end:
	ldmfd	sp!, {r2-r4, pc}
