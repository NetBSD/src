/*	$NetBSD: s3c2410_vector.S,v 1.1 2003/09/03 03:18:30 mycroft Exp $	*/

/*
 * Copyright (c) 2003 By Noon Software, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Vector and initialize for S3C2410 based systems.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/s3c2xx0/s3c2410reg.h>


#define TEMP_STACK_SIZE	(4*1024)

	.code 32
	.section ".vectors"

reset_vector:
	b	__reset_entry
undef:
	b	.
swi:
	b	.
abrtp:
	b	.
abrtd:
	b	.
resv:
	b	.
irq:
	b	.
fiq:
	b	.

/*
 * Normally this code lives on ROM and runs immediately after reset, but
 * it may run on RAM and/or be called after system has been initialized.
 */
__reset_entry:
	mrs	r0, cpsr
	/* SVC mode, Disable interrupts */
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #(I32_bit|F32_bit|PSR_SVC32_MODE)
	msr	cpsr, r0

	/* Disable MMU, Disable cache */
	mrc	p15, 0, r10, c1, c0, 0
	ldr	r0, =(CPU_CONTROL_MMU_ENABLE|CPU_CONTROL_DC_ENABLE|CPU_CONTROL_IC_ENABLE)
	bic	r10, r10, r0
	mcr	p15, 0, r10, c1, c0, 0
	nop
	nop
	nop

	/* invalidate I-cache */
	mcr	p15, 0, r2, c7, c5, 0
	nop
	nop
	nop
	
	/* Enable I-cache */
	orr	r10, r10, #CPU_CONTROL_IC_ENABLE
	mcr	p15, 0, r10, c1, c0, 0
	nop
	nop
	nop

	/* Stop WDT */
	ldr	r0, Lwdt_wtcon_addr
	mov	r1, #WTCON_WDTSTOP
	str	r1, [r0]
		
	/* Disable all interrupts */
	ldr	r0, Lintctl_intmsk_addr
	mov	r1, #0x0
	str	r1, [r0]

	ldr	r9, =S3C2410_GPIO_BASE
	/* LEDs on SMDK2410 */
	mov	r1, #0x5500
	str	r1, [r9, #GPIO_PFCON]
	mov	r1, #0xe0
	str	r1, [r9, #GPIO_PFDAT]
	
	mov	r0, pc
	cmp	r0, #S3C2410_SDRAM_START
	bhs	running_on_ram

	ldr	r8, =S3C2410_CLKMAN_BASE
	ldr	r1, [r8,#CLKMAN_CLKDIVN]
	orr	r1, r1, #CLKDIVN_HDIVN|CLKDIVN_PDIVN
	str	r1, [r8,#CLKMAN_CLKDIVN]

	ldr	r1, Lclkman_locktime_data
	str	r1, [r8,#CLKMAN_LOCKTIME]

	/* Initialize PLL */
	ldr	r1, Lclkman_mpllcon_data
	str	r1, [r8,#CLKMAN_MPLLCON]
	ldr	r1, Lclkman_upllcon_data
	str	r1, [r8,#CLKMAN_UPLLCON]

	ldr	r9, =S3C2410_GPIO_BASE
	mov	r1, #0xd0
	str	r1, [r9, #GPIO_PFDAT]

#ifdef RAM_INIT_HOOK
	bl	RAM_INIT_HOOK
#endif

running_on_ram:	
	/* Change Bus mode to Sync */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1<<31)		/* unset iA bit */
	orr	r0, r0, #(1<<30)		/* set nF bit */
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop

	ldr	r9, =S3C2410_GPIO_BASE
	mov	r1, #0xb0
	str	r1, [r9, #GPIO_PFDAT]

	/* set temporary stack */
	adr	sp, reset_vector
	/* do we have a room below? */
	ldr	r1, =(S3C2410_SDRAM_START+TEMP_STACK_SIZE)
	cmp	sp, r1
	/* otherwise use top area of RAM */
	ldrlo	sp, =(S3C2410_SDRAM_START+SDRAM_SIZE)

#ifdef IO_INIT_HOOK
	bl	IO_INIT_HOOK
#endif

	ldr	r9, =S3C2410_GPIO_BASE
	mov	r1, #0x70
	str	r1, [r9, #GPIO_PFDAT]

	b	start

Lwdt_wtcon_addr:
	.word	(S3C2410_WDT_BASE + WDT_WTCON)

Lintctl_intmsk_addr:
	.word	(S3C2410_INTCTL_BASE + INTCTL_INTMSK)

	.ltorg

Lclkman_locktime_data:
	.word	(0xffffff)

Lclkman_mpllcon_data:
#if XTAL_CLK == 12000000 && FCLK == 180000000
	.word	0x00052011	/* M=82, P=1, S=1 */
#else
#error	illegal XTAL_CLK/FCLK combination
#endif
Lclkman_upllcon_data:
	.word	0x00078023	/* M=120, P=2, S=3 */
