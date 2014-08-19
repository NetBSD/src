
/*
 * Machine dependant startup code for OMAPL138 based Hawk board.
 * Based on omap_start.S
 *
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

/*
 * Machine dependant startup code for OMAPL138 based Hawk board.
 * Based on omap_start.S
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"
	
#include <arm/omap/omapl1x_reg.h>
#include <evbarm/hawk/hawk.h> 	

/*
 * Kernel start routine for HAWK boards.
 * At this point, this code has been loaded into the very beginning of SDRAM
 * and the MMU is off, which implies the data cache is off.
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(hawk_start)
_C_LABEL(hawk_start):

	/*
	 * Save any arguments u-boot passed us.
	 */
	ldr	r4, .Luboot_args
	stmia	r4, {r0-r3}
	
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
	mcr	p15, 0, r0, c7, c5, 0	/* Invalidate I cache */
1:	mrc	p15, 0, r15, c7, c14, 3	/* Test, clean, invalidate D cache. */
	bne	1b
	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffers. */
	mcr	p15, 0, r0, c2, c0, 0	/* Set Translation Table Base */
	mcr	p15, 0, r0, c8, c7, 0	/* Invalidate TLBs */

	/* Set the Domain Access register.  Very important! */
        mov     r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Enable the MMU.
	 */
	ldr	r0, Lcpsr
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c2, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	ldr	pc, Lstart		/* Jump to start (flushes pipeline). */
	/* NOTREACHED */

.Luboot_args:
	.word	uboot_args
Lstart:
	.word	start
Ll1_s_frame:
	.word	L1_S_FRAME
Ltemp_l1_table:
	/* Put the temporary L1 translation table at the end of SDRAM. */
	.word	OMAPL138_MEM_BASE + MEMSIZE_BYTES - L1_TABLE_SIZE
Lcpsr:
	/* What we want to have the CPSR be when we jump to start. */
	.word CPU_CONTROL_MMU_ENABLE | \
	      CPU_CONTROL_AFLT_ENABLE | \
	      CPU_CONTROL_DC_ENABLE | \
	      CPU_CONTROL_WBUF_ENABLE | \
	      CPU_CONTROL_32BP_ENABLE | \
	      CPU_CONTROL_32BD_ENABLE | \
	      CPU_CONTROL_LABT_ENABLE | \
	      CPU_CONTROL_SYST_ENABLE | \
	      CPU_CONTROL_IC_ENABLE

/* We'll modify va and pa at run time so we can use relocatable addresses. */
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	va					    ; \
	.word	pa					    ; \
	.word	n_sec					    ; \
	.word	attr					    ;

mmu_init_table:
	/* Map SDRAM where we're executing from VA==PA, read-only */
	MMU_INIT(KERNEL_BASE_phys, KERNEL_BASE_phys,
		1,
		L1_S_PROTO | L1_S_AP_KR)
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable */
	MMU_INIT(KERNEL_BASE, OMAPL138_MEM_BASE,
		(MEMSIZE_BYTES + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_AP_KRW | L1_S_B | L1_S_C)
	/* Mapping to cover console */
	MMU_INIT(OMAPL1X_KERNEL_IO_VBASE, OMAPL138_IO_BASE,
		1,
		L1_S_PROTO | L1_S_AP_KRW)
	/* end of table */
	MMU_INIT(0, 0, 0, 0)
