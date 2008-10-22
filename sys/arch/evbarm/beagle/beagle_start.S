/*
 * Machine dependant startup code for BEAGLEBOARD boards.
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

#include "opt_omap.h"
#include "opt_com.h"
#include "assym.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#undef DOMAIN_CLIENT	/* assym.h defines as 1, but pte.h defines as 0x01 */
#include <arm/arm32/pmap.h>
#include <arm/omap/omap2_obioreg.h>
#include <evbarm/beagle/beagle.h>  

RCSID("$NetBSD: beagle_start.S,v 1.2 2008/10/22 17:29:33 matt Exp $")

#define Invalidate_I_cache(reg) \
	mcr	p15, 0, reg, c7, c5, 0	/* Invalidate Entire I cache */

/*
 * Kernel start routine for BEAGLEBOARD boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(beagle_start)
_C_LABEL(beagle_start):
	/* Move into supervisor mode and disable IRQs/FIQs. */
	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #(I32_bit | F32_bit | PSR_SVC32_MODE)
	msr	cpsr, r0

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

	/* Now create our entries per the mmu_init_table. */
	l1table	.req r0
	va	.req r1
	pa	.req r2
	n_sec	.req r3
	attr	.req r4
	itable	.req r5
	l1sfrm	.req r6
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
	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	itable
	.unreq	l1table
	.unreq	l1sfrm

	/*
	 * In theory, because the MMU is off, we shouldn't need all of this,
	 * but let's not take any chances and do a typical sequence to set
	 * the Translation Table Base.
	 */

	Invalidate_I_cache(r0)

	mcr	p15, 0, r0, c7, c14, 0	/* Clean and Invalidate Entire Data Cache */

        ldr     r2, Lctl_ID_dis		/* Disable I+D caches */
	mrc	p15, 0, r1, c1, c0, 0	/*  "       "   "     */
	and	r1, r1, r2		/*  "       "   "     */
	mcr	p15, 0, r1, c1, c0, 0	/*  "       "   "     */

	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffers. */
	mcr	p15, 0, r0, c2, c0, 0	/* Set Translation Table Base */
	mcr	p15, 0, r0, c8, c7, 0	/* Invalidate TLBs */

	/* Set the Domain Access register.  Very important! */
        mov     r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

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

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	b	start		/* Jump to start (flushes pipeline). */
	nop
	nop
	nop
	nop
	
	/* NOTREACHED */

Ll1_s_frame:
	.word	L1_S_FRAME
Ltemp_l1_table:
	/* Put the temporary L1 translation table at the end of SDRAM. */
	.word	0x80000000 + MEMSIZE * 0x100000 - L1_TABLE_SIZE

/*
 * Coprocessor register initialization values
 */
# define CPU_AUXCTL_CZ	(1 << 6)		/* Restrict Cache Size */

	/* bits to set in the Control Register */
Lcontrol_set:
	.word CPU_CONTROL_MMU_ENABLE  | \
	      CPU_CONTROL_AFLT_ENABLE | \
	      CPU_CONTROL_DC_ENABLE   | \
	      CPU_CONTROL_WBUF_ENABLE | \
	      CPU_CONTROL_32BP_ENABLE | \
	      CPU_CONTROL_32BD_ENABLE | \
	      CPU_CONTROL_LABT_ENABLE | \
	      CPU_CONTROL_SYST_ENABLE | \
	      CPU_CONTROL_IC_ENABLE

	/* bits to clear in the Control Register */
Lcontrol_clr:
	.word	0

	/* bits to "write as existing" in the Control Register */
Lcontrol_wax:
	.word	(3 << 30) | \
		(1 << 29) | \
		(1 << 28) | \
		(3 << 26) | \
		(3 << 19) | \
		(1 << 17)
		
	/* bits to disable the caches */
Lctl_ID_dis:
	.word	~(CPU_CONTROL_IC_ENABLE|CPU_CONTROL_DC_ENABLE)

	/* bit to restrict cache size */
Lauxctl_CZ_restrict:
	.word	CPU_AUXCTL_CZ


/* We'll modify va and pa at run time so we can use relocatable addresses. */
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	va					    ; \
	.word	pa					    ; \
	.word	n_sec					    ; \
	.word	attr					    ;

mmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable */
	MMU_INIT(KERNEL_BASE, KERNEL_BASE,
		(MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_AP(AP_KRW) | L1_S_B | L1_S_C)

	/* Map first 1MB of L4 CORE (so console will work) */
	MMU_INIT(OMAP3530_L4_CORE_VBASE, OMAP3530_L4_CORE_BASE,
		1,
		L1_S_PROTO | L1_S_AP(AP_KRW))

	/* Map first(all) 1MB of L4 PERIPHERAL (so console will work) */
	MMU_INIT(OMAP3530_L4_PERIPHERAL_VBASE, OMAP3530_L4_PERIPHERAL_BASE,
		1,
		L1_S_PROTO | L1_S_AP(AP_KRW))

	/* Map all 256KB of L4 WAKEUP (so console will work) */
	MMU_INIT(OMAP3530_L4_WAKEUP_VBASE, OMAP3530_L4_WAKEUP_BASE,
		1,
		L1_S_PROTO | L1_S_AP(AP_KRW))

	/* end of table */
	MMU_INIT(0, 0, 0, 0)
