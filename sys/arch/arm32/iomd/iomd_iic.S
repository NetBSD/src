/*	$NetBSD: iomd_iic.S,v 1.2.18.1 1999/12/27 18:31:43 wrstuden Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iic.s
 *
 * Low level routines to with IIC devices
 *
 * Created      : 13/10/94
 *
 * Based of kate/display/iic.s
 */

#include <machine/asm.h>
#include <machine/cpu.h>
#include <arm32/iomd/iomdreg.h>

#define IIC_BITDELAY 10

ENTRY(iic_set_state)
	/*
	 * Parameters
	 *   r0 - IIC data bit
	 *   r1 - IIC clock bit
	 */

	/* Store temporary register */
/*	stmfd	sp!, {r4}*/

	/*
	 * Mask the data and clock bits
	 * Since these routines are only called from iiccontrol.c this is not
	 * really needed
	 */
	and	r0, r0, #0x00000001
	and	r1, r1, #0x00000001

	/* Get address of IOMD control register */

	mov	r2, #(IOMD_BASE)

	/* Get the current CPSR */
/*	mrs     r4, cpsr_all
	orr	r3, r4, #(I32_bit | F32_bit)
	msr	cpsr_all, r3
*/

	IRQdisable
 
	/* Get current value of control register */

	ldrb	r3, [r2, #(IOMD_IOCR << 2)]

	/* Preserve non-IIC bits */

	bic	r3, r3, #0x00000003
#ifdef RC7500
	and	r3, r3, #0x000000ff
#else
/*	orr	r3, r3, #0x000000c0*/
#endif

	/* Set the IIC clock and data bits */

	orr	r3, r3, r0
	orr	r3, r3, r1, lsl #1

	/* Store the new value of control register */

	strb	r3, [r2, #(IOMD_IOCR << 2)]

	/* Restore CPSR state */
/*	msr     cpsr_all, r4 */

	IRQenable

	/* Restore temporary register */
/*	ldmfd	sp!, {r4} */

	/* Pause a bit */

	mov	r0, #(IIC_BITDELAY)

	/* Exit via iic_delay routine */
	b	_C_LABEL(iic_delay)

ENTRY(iic_set_state_and_ack)
	/*
	 * Parameters
	 *  r0 - IIC data bit
	 *  r1 - IIC clock bit
	 */

	/* Store temporary register */
/*	stmfd	sp!, {r4} */

	/*
	 * Mask the data and clock bits
	 * Since these routines are only called from iiccontrol.c this is not
	 * really needed
	 */

	and	r0, r0, #0x00000001
	and	r1, r1, #0x00000001

	/* Get address of IOMD control register */

	mov	r2, #(IOMD_BASE)

	/* Get the current CPSR */
/*	mrs     r4, cpsr_all
	orr	r3, r4, #(I32_bit | F32_bit)
	msr	cpsr_all, r3
*/
	IRQdisable

	/* Get current value of control register */

	ldrb	r3, [r2, #(IOMD_IOCR << 2)]

	/* Preserve non-IIC bits */

	bic	r3, r3, #0x00000003
#ifdef RC7500
	and	r3, r3, #0x000000ff
#else
/*	orr	r3, r3, #0x000000c0*/
#endif

	/* Set the IIC clock and data bits */

	orr	r3, r3, r0
	orr	r3, r3, r1, lsl #1

	/* Store the new value of control register */

	strb	r3, [r2, #(IOMD_IOCR << 2)]

Liic_set_state_and_ack_loop:
	ldrb	r3, [r2, #(IOMD_IOCR << 2)]
	tst	r3, #0x00000002
	beq	Liic_set_state_and_ack_loop

	/* Restore CPSR state */
/*	msr     cpsr_all, r4 */

	IRQenable

	/* Restore temporary register */
/*	ldmfd	sp!, {r4} */

	/* Pause a bit */

	mov	r0, #(IIC_BITDELAY)

	/* Exit via iic_delay routine */
	b	_C_LABEL(iic_delay)


ENTRY(iic_delay)
	/*
	 * Parameters
	 *   r0 - time to wait
	 */

	/* Load address of IOMD */

	mov	r2, #(IOMD_BASE)

	/* Latch current value of timer 1 */

	strb	r2, [r2, #(IOMD_T0LATCH << 2)]

	/* Get the latched value */

	ldrb	r1, [r2, #(IOMD_T0LOW << 2)]

	/* Loop until timer reaches end value */

Liic_delay_loop:

	/* Latch the current value of timer1 */

	strb	r2, [r2, #(IOMD_T0LATCH << 2)]

	/* Get the latched value */

	ldrb	r3, [r2, #(IOMD_T0LOW << 2)]

	/*  Loop until timer reached expected value */

	teq	r3, r1
	movne	r1, r3
	beq	Liic_delay_loop

	subs	r0, r0, #0x00000001
	bne	Liic_delay_loop

	/* Exit */
	mov	pc, lr

ENTRY(iic_getstate)
	/* Load address of IOMD */

	mov	r2, #(IOMD_BASE)

	/* Read IOCR */

	ldrb	r0, [r2, #(IOMD_IOCR << 2)]
	mov	pc, lr

/* End of iomd_iic.S */
