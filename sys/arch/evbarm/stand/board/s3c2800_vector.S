/*	$NetBSD: s3c2800_vector.S,v 1.1 2003/07/30 18:54:23 bsh Exp $	*/

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Vector and initialize for S3C2800 based systems.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/s3c2xx0/s3c2800reg.h>

#ifndef PLLCON_MDIV_VAL
/* constans to get 200MHz FCLK */
#if XTAL_CLK == 10000000 || XTAL_CLK == 10
#define PLLCON_MDIV_VAL	0x5c
#define PLLCON_PDIV_VAL	3
#define PLLCON_SDIV_VAL	0
#elif XTAL_CLK == 8000000 || XTAL_CLK == 8
#define PLLCON_MDIV_VAL	0x5c
#define PLLCON_PDIV_VAL	2
#define PLLCON_SDIV_VAL	0
#elif XTAL_CLK == 6000000 || XTAL_CLK == 6
#define PLLCON_MDIV_VAL	0x5c
#define PLLCON_PDIV_VAL	1
#define PLLCON_SDIV_VAL	0
#else
#error define XTAL_CLK to 10, 8 or 6MHz
#endif	/* XTAL_CLK */
#endif	/* PLLCON_MDIV_VAL */
	
#ifndef SDRAM_START
#define SDRAM_START	S3C2800_DBANK0_START
#endif
#ifndef SDRAM_SIZE
#define SDRAM_SIZE	0x01000000			    /* 16MB */
#endif

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

#if 0
	ldr	r9, =S3C2800_GPIO_BASE
	/* LEDs on SMDK2800 */
	mov	r1, #0x3f
	strh	r1, [r9, #GPIO_PCONC]
	
	mov	r0, #0xdfff	    /* set PB7 to AHBCLK out */
	strh	r0, [r9,GPIO_PCONB]
#endif

	mov	r0, pc
	cmp	r0, #SDRAM_START
	bhs	running_on_ram

#ifdef RAM_INIT_HOOK
	bl	RAM_INIT_HOOK
#endif
	ldr	r8, =S3C2800_CLKMAN_BASE
	ldr	r1, [r8,#CLKMAN_CLKCON]
	orr	r1, r1, #CLKCON_HCLK  /* AHB clock = FCLK/2 */
	str	r1, [r8,#CLKMAN_CLKCON]

	ldr	r1, Lclkman_locktime_data
	str	r1, [r8,#CLKMAN_LOCKTIME]

	/* Initialize PLL */
	ldr	r1, Lclkman_pllcon_data
	str	r1, [r8,#CLKMAN_PLLCON]

running_on_ram:	
	/* Change Bus mode to Sync */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1<<31)		/* unset iA bit */
	orr	r0, r0, #(1<<30)		/* set nF bit */
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop

	/* set temporary stack */
	adr	sp, reset_vector
	/* do we have a room below? */
	ldr	r1, =(SDRAM_START+TEMP_STACK_SIZE)
	cmp	sp, r1
	/* otherwise use top area of RAM */
	ldrlo	sp, =(SDRAM_START+SDRAM_SIZE)

#ifdef IO_INIT_HOOK
	bl	IO_INIT_HOOK
#endif
#ifdef SELFCOPY_TO_FLASH
	/* Are we running on RAM? */
	mov	r0, pc
	cmp	r0, #SDRAM_START
	blo	99f      /* no, skip */
	adr	r0, reset_vector
	ldr	r1, =__rom_size__
	b	selfcopy_to_flash
99:	
#endif
	b	start

Lwdt_wtcon_addr:
	.word	(S3C2800_WDT_BASE + WDT_WTCON)

Lintctl_intmsk_addr:
	.word	(S3C2800_INTCTL_BASE + INTCTL_INTMSK)

	.ltorg

Lclkman_locktime_data:
	.word	(0xfff)

Lclkman_pllcon_data:
	.word	(PLLCON_MDIV_VAL<<PLLCON_MDIV_SHIFT) | \
		(PLLCON_PDIV_VAL<<PLLCON_PDIV_SHIFT) | \
		(PLLCON_SDIV_VAL<<PLLCON_SDIV_SHIFT)

