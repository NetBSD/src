/*	$NetBSD: ixdp425_start.S,v 1.2 2003/05/24 01:59:32 ichiro Exp $ */

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

#include <arm/xscale/ixp425reg.h>

	.section .start,"ax",%progbits

	.global	_C_LABEL(ixdp425_start)
_C_LABEL(ixdp425_start):
        /*
         * We will go ahead and disable the MMU here so that we don't
         * have to worry about flushing caches, etc.
         *
         * Note that we may not currently be running VA==PA, which means
         * we'll need to leap to the next insn after disabing the MMU.
         */
        adr     r8, Lunmapped
        bic     r8, r8, #0xff000000     /* clear upper 8 bits */
        orr     r8, r8, #0x10000000     /* OR in physical base address */

	/*
	 * Setup coprocessor 15.
	 */
	/*
	 *IXDP425 with CSR(microengine code produced by Intel Corp.)
	 *  running well on BigEndian, because CSR written on bigendian
	 */
        mrc     p15, 0, r2, c1, c0, 0
        bic     r2, r2, #CPU_CONTROL_MMU_ENABLE
	orr	r2, r2, #CPU_CONTROL_BEND_ENABLE
        mcr     p15, 0, r2, c1, c0, 0

        nop
        nop
        nop

        mov     pc, r8                  /* Heave-ho! */

Lunmapped:
	/*
	 * We want to construct a memory map that maps us
	 * VA==PA (SDRAM at 0x10000000). We create these
	 * mappings uncached and unbuffered to be safe.
	 */

	/*
	 * Step 1: Map the entire address space VA==PA.
	 */
	adr	r0, Ltable
	ldr	r0, [r0]			/* r0 = &l1table */

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
         * Step 2: Map VA 0xc0000000->0xc3ffffff to PA 0x10000000->0x13ffffff.
         */
        adr     r0, Ltable                      /* r0 = &l1table */
        ldr     r0, [r0]

        mov     r3, #(L1_S_AP(AP_KRW))
        orr     r3, r3, #(L1_TYPE_S)
        orr     r3, r3, #0x10000000
        add     r0, r0, #(0xc00 * 4)            /* offset to 0xc00xxxxx */
        mov     r1, #0x40                       /* 64MB */
1:
        str     r3, [r0], #0x04
        add     r3, r3, r2
        subs    r1, r1, #1
        bgt     1b

	/*
	 * Step 3: Map VA 0xf0000000->0xf0100000 to PA 0xc8000000->0xc8100000.
	 */
	adr	r0, Ltable			/* r0 = &l1table */
	ldr	r0, [r0]

	add	r0, r0, #(0xf00 * 4)		/* offset to 0xf0000000 */
	mov	r3, #0xc8000000
	add	r3, r3, #0x00100000
	orr	r3, r3, #(L1_S_AP(AP_KRW))
	orr	r3, r3, #(L1_TYPE_S)
	str	r3, [r0]

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
	ldr	r0, Lstart

	/* OK, let's enable the MMU. */
	mrc	p15, 0, r2, c1, c0, 0
	orr	r2, r2, #CPU_CONTROL_MMU_ENABLE
	orr	r2, r2, #CPU_CONTROL_BEND_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop

	/* CPWAIT sequence to make sure the MMU is on... */
	mrc	p15, 0, r2, c2, c0, 0	/* arbitrary read of CP15 */
	mov	r2, r2			/* force it to complete */
	mov	pc, r0			/* leap to kernel entry point! */

Ltable:
	.word	0x10200000 - 0x4000

Lstart:
	.word	start
