/*	$NetBSD: brh_start.S,v 1.2.14.1 2014/08/20 00:02:52 tls Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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


#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/xscale/beccreg.h>

#include <evbarm/adi_brh/brhreg.h>

RCSID("$NetBSD: brh_start.S,v 1.2.14.1 2014/08/20 00:02:52 tls Exp $")

	.section .start,"ax",%progbits

	.global	_C_LABEL(brh_start)
_C_LABEL(brh_start):
	/*
	 * Get a pointer to the LED (physical address).
	 */
	mov	ip, #(BRH_LED_BASE)

	/*
	 * We will go ahead and disable the MMU here so that we don't
	 * have to worry about flushing caches, etc.
	 *
	 * Note that we may not currently be running VA==PA, which means
	 * we'll need to leap to the next insn after disabing the MMU.
	 */
	adr	r8, .Lunmapped
	bic	r8, r8, #0xff000000	/* clear upper 8 bits */
	orr	r8, r8, #0xc0000000	/* OR in physical base address */

	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	mov	pc, r8			/* Heave-ho! */

.Lunmapped:
	/*
	 * We want to construct a memory map that maps us
	 * VA==PA (SDRAM at 0xc0000000) (which also happens
	 * to be where the kernel address space starts).
	 * We create these mappings uncached and unbuffered
	 * to be safe.
	 *
	 * We also map various devices at their expected locations,
	 * because we will need to talk to them during bootstrap.
	 *
	 * We just use section mappings for all of this to make it easy.
	 *
	 * We will put the L1 table to do all this at 0xc0004000.
	 */


	/*
	 * Step 1: Map the entire address space VA==PA.
	 */
	ldr	r0, .Ltable			/* r0 = &l1table */

	mov	r3, #(L1_S_AP_KRW)
	orr	r3, r3, #(L1_TYPE_S)
	mov	r2, #0x100000			/* advance by 1MB */
	mov	r1, #0x1000			/* 4096MB */
1:
	str	r3, [r0], #0x04
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

#if 0
	/*
	 * Step 2: Map the PCI configuration space (this is needed
	 * to access some of the core logic registers).
	 */
	ldr	r0, .Ltable			/* r0 = &l1table */

	mov	r3, #(L1_S_AP_KRW)
	orr	r3, r3, #(L1_TYPE_S)
	orr	r3, r3, #(BECC_PCI_CONF_BASE)
	add	r0, r0, #((BRH_PCI_CONF_VBASE >> L1_S_SHIFT) * 4)
	mov	r1, #(BRH_PCI_CONF_VSIZE >> L1_S_SHIFT)
1:
	str	r3, [r0], #0x04
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b
#endif

	/*
	 * Step 3: Map the BECC, UARTs, and LED display.
	 */
	ldr	r0, .Ltable			/* r0 = &l1table */

	mov	r3, #(L1_S_AP_KRW)
	orr	r3, r3, #(L1_TYPE_S)

	orr	r3, r3, #(BECC_REG_BASE)
	ldr	r2, .Lbrh_becc_vbase
	str	r3, [r0, r2]
	bic	r3, r3, #(BECC_REG_BASE)

	orr	r3, r3, #(BRH_UART1_BASE)
	ldr	r2, .Lbrh_uart1_vbase 
	str	r3, [r0, r2]
	bic	r3, r3, #(BRH_UART1_BASE)

	orr	r3, r3, #(BRH_UART2_BASE)
	ldr	r2, .Lbrh_uart2_vbase 
	str	r3, [r0, r2]
	bic	r3, r3, #(BRH_UART2_BASE)

	orr	r3, r3, #(BRH_LED_BASE)
	ldr	r2, .Lbrh_led_vbase 
	str	r3, [r0, r2]
	bic	r3, r3, #(BRH_LED_BASE)

	/* OK!  Page table is set up.  Give it to the CPU. */
	ldr	r0, .Ltable
	mcr	p15, 0, r0, c2, c0, 0

	/* Flush the old TLBs, just in case. */
	mcr	p15, 0, r0, c8, c7, 0

	/* Set the Domain Access register.  Very important! */
	mov	r0, #1
	mcr	p15, 0, r0, c3, c0, 0

	/* Get ready to jump to the "real" kernel entry point... */
	ldr	r0, .Lstart

	/* OK, let's enable the MMU. */
	mrc	p15, 0, r2, c1, c0, 0
	orr	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	/* CPWAIT sequence to make sure the MMU is on... */
	mrc	p15, 0, r2, c2, c0, 0	/* arbitrary read of CP15 */
	mov	r2, r2			/* force it to complete */
	mov	pc, r0			/* leap to kernel entry point! */

.Lbrh_becc_vbase:
	.word	((BRH_BECC_VBASE >> L1_S_SHIFT) * 4)

.Lbrh_uart1_vbase:
	.word	((BRH_UART1_VBASE >> L1_S_SHIFT) * 4)

.Lbrh_uart2_vbase:
	.word	((BRH_UART2_VBASE >> L1_S_SHIFT) * 4)

.Lbrh_led_vbase:
	.word	((BRH_LED_VBASE >> L1_S_SHIFT) * 4)

.Ltable:
	.word	0xc0004000

.Lstart:
	.word	start
