/*	$NetBSD: if_eca_fiq.S,v 1.3 2001/09/22 17:19:27 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

RCSID("$NetBSD: if_eca_fiq.S,v 1.3 2001/09/22 17:19:27 bjh21 Exp $")

#include <dev/ic/mc6854reg.h>
#include <arch/arm26/ioc/if_ecavar.h>

#include "assym.h"

ENTRY_NP(eca_fiqhandler)
	ldr	pc, [r11, #EFS_FIQHANDLER]
	.global	eca_efiqhandler
_C_LABEL(eca_efiqhandler):
	
/*
 * Econet Rx FIQ handler registers:
 * R8:  Address of 6854
 * R9:  Data buffer address
 * R10: Space left in buffer
 * R11: struct eca_fiqstate pointer
 * R12: Scratch
 * R13: Scratch
 */

ENTRY(eca_fiqhandler_rx)
	/* If there's something in the Rx FIFO, read it now. */
	ldrb	r12, [r8, #(MC6854_SR2 << 2)]
	tst	r12, #MC6854_SR2_RDA
	beq	Leca_rx_nodata
Leca_rx_loop:
	ldrb	r12, [r8, #(MC6854_RXFIFO << 2)]
	strb	r12, [r9], #1
	subs	r10, r10, #1
	beq	Leca_rx_counter		/* Rx buffer full */
	ldrb	r12, [r8, #(MC6854_SR2 << 2)]
	tst	r12, #MC6854_SR2_RDA	/* More data? */
	bne	Leca_rx_loop
Leca_rx_nodata:
	teq	r12, #0			/* No more status? */
	subeqs	pc, r14, #4		/* Return. */
	tst	r12, #MC6854_SR2_FV	/* End of frame? */
	ldrne	r12, [r11, #EFS_RX_FLAGS]
	tstne	r12, #ERXF_FLAGFILL	/* Want flag fill? */
	movne	r12, #(MC6854_CR2_RTS | MC6854_CR2_F_M_IDLE)
	strneb	r12, [r8, #(MC6854_CR2 << 2)]
	b	fiq_downgrade

Leca_rx_counter:
	/* If we've already got the header, this indicates end-of-buffer. */
	ldr	r12, [r11, #EFS_RX_FLAGS]
	tst	r12, #ERXF_GOTHDR
	bne	Leca_rx_full
	ldrb	r12, [r9, #-2]
	ldrb	r13, [r11, #EFS_RX_MYADDR]
	teq	r12, r13		/* Our host */
	ldreqb	r12, [r9, #-1]
	teqeq	r12, #0			/* Local network? */
	ldr	r12, [r11, #EFS_RX_FLAGS]
	orrne	r12, r12, #ERXF_GOTHDR
	orreq	r12, r12, #(ERXF_GOTHDR | ERXF_FLAGFILL)
	str	r12, [r11, #EFS_RX_FLAGS]
	ldr	r12, [r11, #EFS_RX_CURMBUF]
	ldr	r10, [r12, #M_LEN]
	ldr	r12, [r12, #M_DATA]
	sub	r12, r9, r12		/* Amount got already */
	sub	r10, r10, r12
	subs	pc, r14, #4

Leca_rx_full:
	/* Rx buffer full.  See if there's another mbuf in the chain. */
	ldr	r12, [r11, #EFS_RX_CURMBUF]
	ldr	r12, [r12, #M_NEXT]
	str	r12, [r11, #EFS_RX_CURMBUF]
	teq	r12, #0
	beq	fiq_downgrade
	ldr	r9, [r12, #M_DATA]
	ldr	r10, [r12, #M_LEN]
	subs	pc, r14, #4

/*
 * Econet Tx FIQ handler registers:
 * R8:  Address of 6854
 * R9:  Data buffer address
 * R10: Data left in buffer
 * R11: struct eca_fiqstate pointer
 * R12: Scratch
 * R13: Scratch
 */
ENTRY(eca_fiqhandler_tx)
	ldrb	r12, [r8, #(MC6854_SR1 << 2)]
	tst	r12, #MC6854_SR1_TDRA
	beq	Leca_tx_nospace
Leca_tx_loop:
	ldrb	r12, [r9], #1
	strb	r12, [r8, #(MC6854_TXFIFOFC << 2)]
	subs	r10, r10, #1
	beq	Leca_tx_nodata
	ldrb	r12, [r8, #(MC6854_SR1 << 2)]
	tst	r12, #MC6854_SR1_TDRA
	bne	Leca_tx_loop
Leca_tx_nospace:
	tst	r12, #MC6854_SR1_IRQ	/* No more status? */
	subeqs	pc, r14, #4		/* Return. */
	b	fiq_downgrade

Leca_tx_nodata:
	/* We get here when the current data block is empty. */
	ldr	r12, [r11, #EFS_TX_CURMBUF]
	ldr	r12, [r12, #M_NEXT]
	str	r12, [r11, #EFS_TX_CURMBUF]
	teq	r12, #0				/* Another mbuf? */
	beq	Leca_tx_endofframe
	ldr	r9, [r12, #M_DATA]		/* Line up next mbuf. */
	ldr	r10, [r12, #M_LEN]
	subs	pc, r14, #4

Leca_tx_endofframe:
	mov	r12, #(MC6854_CR2_TX_LAST)	/* If not, finish frame... */
	strb	r12, [r8, #(MC6854_CR2 << 2)]
	/* Code equivalent to parts of eca_init_rx_hard() */
	adr	r12, _C_LABEL(eca_fiqhandler_rx)
	str	r12, [r11, #(EFS_FIQHANDLER)]
	mov	r12, #(MC6854_CR1_RIE)
	strb	r12, [r8, #(MC6854_CR1 << 2)]
	add	r12, r11, #(EFS_RX_FIQREGS)
	ldmia	r12, {r8-r11}
	/* End code equivalent to parts of eca_init_rx_hard() */
	b	fiq_downgrade_dont_disable	/* ... and report back. */
