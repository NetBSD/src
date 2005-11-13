/*	$NetBSD: armadillo9_start.S,v 1.1 2005/11/13 06:33:05 hamajima Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>

	.section .start,"ax",%progbits

	.global	_C_LABEL(armadillo9_start)
_C_LABEL(armadillo9_start):

	/* make sure svc mode and all fiqs&irqs disabled */
	mov	r0, #(PSR_SVC32_MODE | I32_bit | F32_bit)
	msr	cpsr_c, r0

	/*
	 * We will go ahead and disable the MMU here so that we don't
	 * have to worry about flushing caches, etc.
	 *
	 * Note that we may not currently be running VA==PA, which means
	 * we'll need to leap to the next insn after disabing the MMU.
	 */
	adr	r8, Lunmapped
	bic	r8, r8, #0xff000000	/* clear upper 8 bits */
	orr	r8, r8, #0xc0000000	/* OR in physical base address */

	/*
	 * Setup coprocessor 15.
	 */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE
	bic	r2, r2, #CPU_CONTROL_IC_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop
	mov	pc, r8			/* Heave-ho! */

Lunmapped:
#ifdef VERBOSE_INIT_ARM
	/* set temporary stack pointer */
	adr	sp, Ltable
	ldr	sp, [sp]

	/* initialize UART1 */
	bl	init_UART1
#endif
	/* copy to virtual address */
	adr	r0, Lsection
	ldmia	r0, {r0, r1, r2}	/* r0: kernel(load) start address */
					/* r1: kernel(virtual) start address */
					/* r2: kernel(virtual) end address */
	sub	r3, r2, r1		/* r3: kernel size */
	add	r4, r0, r3		/* r4: kernel(load) end address */
#ifdef VERBOSE_INIT_ARM
#define	putc(c)	 \
	mov	r5, c	;\
	bl	print_char
	putc(#'c')
	putc(#'o')
	putc(#'p')
	putc(#'y')
	putc(#' ')
	putc(#'f')
	putc(#'r')
	putc(#'o')
	putc(#'m')
	putc(#' ')
	bl	print_r0
	putc(#' ')
	putc(#'t')
	putc(#'o')
	putc(#' ')
	bl	print_r1
	putc(#' ')
	putc(#'s')
	putc(#'i')
	putc(#'z')
	putc(#'e')
	putc(#' ')
	bl	print_r3
	bl	print_cr
#endif
1:
	ldr	r5, [r4], #-4
	str	r5, [r2], #-4
	cmp	r4, r0
	bge	1b

	/*
	 * We want to construct a memory map that maps us
	 * VA==PA (SDRAM at 0xc0000000). We create these
	 * mappings uncached and unbuffered to be safe.
	 */
	/*
	 * Step 1: Map the entire address space VA==PA.
	 */
	adr	r4, Ltable
	ldr	r0, [r4]			/* r0 = &l1table */
	mov	r1, #(L1_TABLE_SIZE / 4)	/* 4096 entry */
	mov	r2, #(L1_S_SIZE)		/* 1MB / section */
	mov	r3, #(L1_S_AP(AP_KRW))		/* kernel read/write */
	orr	r3, r3, #(L1_TYPE_S)		/* L1 entry is section */
1:
	str	r3, [r0], #0x04
	add	r3, r3, r2
	subs	r1, r1, #1
	bgt	1b

	/*
	 * Step 2: Map VA 0xf0000000->0xf00fffff to PA 0x80000000->0x800fffff.
	 */
	ldr	r0, [r4]
	add	r0, r0, #(0xf00 * 4)		/* offset to 0xf0000000 */
	mov	r3, #(L1_S_AP(AP_KRW))		/* kernel read/write */
	orr	r3, r3, #(L1_TYPE_S)		/* L1 entry is section */
	orr	r3, r3, #0x80000000
	str	r3, [r0], #4

	/*
	 * Step 3: Map VA 0xf0100000->0xf02fffff to PA 0x80800000->0x809fffff.
	 */
	mov	r3, #(L1_S_AP(AP_KRW))		/* kernel read/write */
	orr	r3, r3, #(L1_TYPE_S)		/* L1 entry is section */
	orr	r3, r3, #0x80000000
	orr	r3, r3, #0x00800000
	str	r3, [r0], #0x4
	add	r3, r3, r2
	str	r3, [r0], #0x4

	/* OK!  Page table is set up.  Give it to the CPU. */
	adr	r0, Ltable
	ldr	r0, [r0]
	mcr	p15, 0, r0, c2, c0, 0

	/* Flush the old TLBs, just in case. */
	mcr	p15, 0, r0, c8, c7, 0

	/* Set the Domain Access register.  Very important! */
	mov	r0, #1
	mcr	p15, 0, r0, c3, c0, 0

	/* Get ready to jump to the "real" kernel entry point... */
	ldr	r1, Lstart
	mov	r1, r1			/* Make sure the load completes! */

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
	mov	pc, r1			/* leap to kernel entry point! */

Ltable:
	.word	armadillo9_start - L1_TABLE_SIZE

Lstart:
	.word	start

Lsection:
	.word	.start
	.word	0xc0200000
	.word	__bss_start

#ifdef VERBOSE_INIT_ARM
init_UART1:
	stmfd	sp!, {r4, r5, lr}
	mov	r4, #0x80000000
	add	r4, r4, #0x008c0000
	ldr	r5, [r4, #0x08]
	orr	r5, r5, #0x10
	str	r5, [r4, #0x08]	/* enable FIFO */
	mov	r5, #0x01
	str	r5, [r4, #0x14]	/* disable interrupt */
	ldmfd	sp!, {r4, r5, pc}

print_char:	/* char = r5 */
	stmfd	sp!, {r4, r6, lr}
	mov	r4, #0x80000000
	add	r4, r4, #0x008c0000
1:
	ldr	r6, [r4, #0x18]
	tst	r6, #0x20	/* check TXFF */
	bne	1b
	str	r5, [r4, #0x00]
	ldmfd	sp!, {r4, r6, pc}

print_cr:
	stmfd	sp!, {r5, lr}
	mov	r5, #0x0d	/* cr */
	bl	print_char
	mov	r5, #0x0a	/* lf */
	bl	print_char
	ldmfd	sp!, {r5, pc}

print_r0:
	stmfd	sp!, {r0, r4-r7, lr}
	mov	r4, #28
	mov	r6, #0xf
1:	
	and	r7, r6, r0, ROR r4
	cmp	r7, #10
	addlt	r5, r7, #'0'
	addge	r5, r7, #('a' - 0x0a)
	bl	print_char
	subs	r4, r4, #4
	bge	1b
	ldmfd	sp!, {r0, r4-r7, pc}

#define	print_register(reg)	 \
	stmfd	sp!, {r0, lr}	;\
	mov	r0, reg		;\
	bl	print_r0	;\
	ldmfd	sp!, {r0, pc}

print_r1:
	print_register(r1)

print_r2:
	print_register(r2)

print_r3:
	print_register(r3)

print_r4:
	print_register(r4)

print_r5:
	print_register(r5)

print_r6:
	print_register(r6)

print_r7:
	print_register(r7)

print_r8:
	print_register(r8)

print_r9:
	print_register(r9)

print_r10:
	print_register(r10)

print_r11:
	print_register(r11)

print_r12:
	print_register(r12)
#endif
