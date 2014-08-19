/*	$NetBSD: gemini_start.S,v 1.8.12.1 2014/08/20 00:02:54 tls Exp $	*/

/*
 * Machine dependent startup code for GEMINI boards.
 * Based on omap_start.S
 *
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_gemini.h"
#include "opt_com.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"

RCSID("$NetBSD: gemini_start.S,v 1.8.12.1 2014/08/20 00:02:54 tls Exp $")


#if defined(VERBOSE_INIT_ARM)
# define _PUTCHAR(addr, areg, breg, c) 			\
	ldr	areg, addr;				\
1:							\
	ldr	breg, [areg, #0x14];	/* LSR    */	\
	tst	breg, #0x20;		/* TXRDY? */	\
	beq	1b;					\
	mov	breg, #(c);		/*   c    */	\
	str	breg, [areg];		/* TXDATA */	\
2:							\
	ldr	breg, [areg, #0x14];	/* LSR    */	\
	tst	breg, #0x40;		/* TSRE?  */	\
	beq	2b;
#else
# define _PUTCHAR(addr, areg, breg, c)
#endif


/*
 * Kernel start routine for GEMINI Eval board.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(gemini_start)
_C_LABEL(gemini_start):
	/* Move into supervisor mode and disable IRQs/FIQs. */
	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #(I32_bit | F32_bit | PSR_SVC32_MODE)
	msr	cpsr, r0

	_PUTCHAR(Lconsole_pbase, r4, r3, 'a')

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	/* Build page table from scratch */
	ldr	r0, Ltemp_l1_table
	mov	r1, r0			/* Save the page table address. */
	/* Zero the entire table so all virtual addresses are invalid. */
	mov	r2, #L1_TABLE_SIZE	/* in bytes */
	mov	r3, #0
	mov	r4, r3
	mov	r5, r3
	mov	r6, r3
	mov	r7, r3
	mov	r8, r3
	mov	r10, r3
	mov	r11, r3
1:	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	subs	r2, r2, #(4 * 4 * 8)	/* bytes per loop */
	bne	1b

	_PUTCHAR(Lconsole_pbase, r4, r3, 'b')
	
	/* Now create our entries per the mmu_init_table. */
	l1table	.req r0
	va	.req r1
	pa	.req r2
	n_sec	.req r3
	attr	.req r4
	itable	.req r5
	l1sfrm	.req r6
	ldr	l1table, Ltemp_l1_table
	adr	itable, mmu_init_table
	ldr	l1sfrm, Ll1_s_frame
	b	3f
2:	str	pa, [l1table, va]
	add	va, va, #4
	add	pa, pa, #(L1_S_SIZE)
	adds	n_sec, n_sec, #-1
	bhi	2b
3:	ldmia	itable!, {va,pa,n_sec,attr}
	/* Convert va to l1 offset:	va = 4 * (va >> L1_S_SHIFT)	*/
	mov	va, va, LSR #L1_S_SHIFT
	mov	va, va, LSL #2
	/* Convert pa to l1 entry:	pa = (pa & L1_S_FRAME) | attr	*/
	and	pa, pa, l1sfrm
	orr	pa, pa, attr
	cmp	n_sec, #0
	bne	2b
	mov	r5, r0			/* l1table */
	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	itable
	.unreq	l1table
	.unreq	l1sfrm

	_PUTCHAR(Lconsole_pbase, r4, r3, 'c')
	
	/*
	 * using FA526 -specific cache ops here...
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5,  0	/* Invalidate Entire I cache */
	mcr	p15, 0, r0, c7, c14, 0	/* Clean & Invalidate Entire D cache */

        ldr     r2, Lctl_ID_dis		/* Disable I+D caches */
	mrc	p15, 0, r1, c1, c0, 0	/*  "       "   "     */
	and	r1, r1, r2		/*  "       "   "     */
	mcr	p15, 0, r1, c1, c0, 0	/*  "       "   "     */

	_PUTCHAR(Lconsole_pbase, r4, r3, 'd')

	mcr	p15, 0, r0, c7, c5, 6	/* invalidate BTB all */
	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffers. */
	mcr	p15, 0, r5, c2, c0, 0	/* Set Translation Table Base */
	mcr	p15, 0, r0, c8, c7, 0	/* Invalidate TLBs */

	/* Set the Domain Access register */
        mov     r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * set Extension Control Enable in ECR, so we can use BTB
	 */
	ldr     r0, Lecr_set
	mcr     p15, 0, r0, c1, c1, 0

	/*
	 * Enable the MMU, etc.
	 */
	mrc     p15, 0, r0, c1, c0, 0
	ldr     r1, Lcontrol_wax
	and	r0, r0, r1
	ldr     r1, Lcontrol_clr
	mvn	r1, r1
	and	r0, r0, r1
	ldr     r1, Lcontrol_set
	orr	r0, r0, r1
	mcr     p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c2, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */

	_PUTCHAR(Luart_vbase, r4, r3, 'e')

	/*
	 * Zero .bss
	 */
	ldr	r0, L_edata
	ldr	r1, L_end
	mov	r2, #0
1:
	str	r2, [r0], #0x04		/* *r0++ = r2 */
	cmp	r0, r1
	bne	1b

#if 0
	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	adr	r0, Ltestjmp
	ldr	pc, [r0]
	nop
	nop
	nop
	nop
testjmp:
#endif

	_PUTCHAR(Luart_vbase, r4, r3, 'f')

	adr	r0, Lstart
	ldr	pc, [r0]
	nop
	nop
	nop
	nop
	
	/* NOTREACHED */

L_edata:
	.word   _C_LABEL(_edata)
L_end:
	.word   _C_LABEL(_end)

#if 0
Ltestjmp:
	.word	testjmp
#endif

Lstart:
	.word	start
Ll1_s_frame:
	.word	L1_S_FRAME
Ltemp_l1_table:
	/* Put the temporary L1 translation table at the end of SDRAM. */
	.word	MEMSIZE * 0x100000 - L1_TABLE_SIZE

/*
 * Coprocessor register initialization values
 */
#if !defined(CPU_ECR_ECE)
# define	CPU_ECR_ECE		1
#endif
	/* bits to set in the Extension Control Register */
Lecr_set:
	.word	CPU_ECR_ECE

#if !defined(CPU_CONTROL_BTB_ENABLE)
# define	CPU_CONTROL_BTB_ENABLE	(1 << 11)
#endif
	/* bits to set in the Control Register */
	/* bits 6..4 SB1 */
Lcontrol_set:
	.word CPU_CONTROL_MMU_ENABLE  | \
	      CPU_CONTROL_AFLT_ENABLE | \
	      CPU_CONTROL_DC_ENABLE   | \
	      CPU_CONTROL_WBUF_ENABLE | \
	      CPU_CONTROL_32BP_ENABLE | \
	      CPU_CONTROL_32BD_ENABLE | \
	      CPU_CONTROL_LABT_ENABLE | \
	      CPU_CONTROL_SYST_ENABLE | \
	      CPU_CONTROL_IC_ENABLE   | \
	      CPU_CONTROL_DC_ENABLE   | \
	      CPU_CONTROL_BTB_ENABLE

	/* bits to clear in the Control Register */
	/* bits 31..14, 10,  SBZ */
Lcontrol_clr:
	.word	((~0) << 14) | \
		(1 << 10)

	/* bits to "write as existing" in the Control Register */
Lcontrol_wax:
	.word	CPU_CONTROL_BEND_ENABLE
		
	/* bits to disable the caches */
Lctl_ID_dis:
	.word	~(CPU_CONTROL_IC_ENABLE|CPU_CONTROL_DC_ENABLE)

	/* console addressing */
Lconsole_pbase:
#if 0
	.word	CONSADDR
#else
	.word	GEMINI_UART_BASE
#endif
Luart_vbase:
	.word	GEMINI_UART_VBASE


/* We'll modify va and pa at run time so we can use relocatable addresses. */
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	va					    ; \
	.word	pa					    ; \
	.word	n_sec					    ; \
	.word	attr					    ;

mmu_init_table:
	/* Maintain current 1:1 addressability */
	MMU_INIT(KERNEL_BASE_phys, KERNEL_BASE_phys,
		(MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_AP_KRW | L1_S_B | L1_S_C)

	/* Map Kernel base VA:PA, write-back cacheable */
	MMU_INIT(KERNEL_BASE_virt, KERNEL_BASE_phys,
		(MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_AP_KRW | L1_S_B | L1_S_C)

	/* Map Gemini GLOBAL regs */
	MMU_INIT(GEMINI_GLOBAL_VBASE, GEMINI_GLOBAL_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)

	/* Map Gemini UART */
	MMU_INIT(GEMINI_UART_VBASE, GEMINI_UART_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)

	/* Map Gemini LPC Host Controlr Space */
	MMU_INIT(GEMINI_LPCHC_VBASE, GEMINI_LPCHC_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)

	/* Map Gemini LPC IO Space */
	MMU_INIT(GEMINI_LPCIO_VBASE, GEMINI_LPCIO_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)

	/* Map Gemini DRAM Controller Space */
	MMU_INIT(GEMINI_DRAMC_VBASE, GEMINI_DRAMC_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

