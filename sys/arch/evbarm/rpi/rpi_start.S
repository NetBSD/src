/*	$NetBSD: rpi_start.S,v 1.5.2.2 2014/08/20 00:02:56 tls Exp $	*/

/*
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

#include "opt_cputypes.h"
#include "opt_cpuoptions.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"

RCSID("$NetBSD: rpi_start.S,v 1.5.2.2 2014/08/20 00:02:56 tls Exp $")

/*
 * Workaround Erratum 411920
 *
 *	- value of arg 'reg' Should Be Zero
 */
#define Invalidate_I_cache(reg) \
	.p2align 5;								\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;

/*
 * Kernel start routine for RPI boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.text

	.global	_C_LABEL(rpi_start)
_C_LABEL(rpi_start):
	adr	r8, rpi_boot_regs
	stmia	r8!, {r0-r3}

	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #(I32_bit | F32_bit | PSR_SVC32_MODE)
	msr	cpsr, r0

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	/* Build page table from scratch */
	ldr	r0, Ltemp_l1_table	/* The page table address - entered into TTB later */
	mov	r1, r0			/* Start address to clear memory. */
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
	mov	n_sec, n_sec, lsr #L1_S_SHIFT
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
	mov	r0, #0			/* SBZ */
	Invalidate_I_cache(r0)

	mcr	p15, 0, r0, c7, c14, 0	/* Clean and Invalidate Entire Data Cache */

	ldr     r2, Lctl_ID_dis		/* Disable I+D caches */
	mrc	p15, 0, r1, c1, c0, 0	/*  "       "   "     */
	and	r1, r1, r2		/*  "       "   "     */
	mcr	p15, 0, r1, c1, c0, 0	/*  "       "   "     */

	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffers. */

	ldr	r0, Ltemp_l1_table	/* The page table address */

	mcr	p15, 0, r0, c2, c0, 0	/* Set Translation Table Base 0 (TTB0) */

#if defined(ARM_MMU_EXTENDED)
	// When using split TTBRs, we need to set both since the physical
	// addresses we were/are using might be in either.
	mcr	p15, 0, r0, c2, c0, 1	/* TTBR1 write */
#endif  

#if defined(ARM_MMU_EXTENDED)
	mov	r1, #TTBCR_S_N_1        /* make sure TTBCR_S_N is 1 */
#else   
	mov	r1, #0			/* make sure TTBCR is 0 */
#endif
	mcr	p15, 0, r1, c2, c0, 2	/* TTBCR write */
  
	mov     r0, #0
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
	bic	r0, r0, r1
	ldr     r1, Lcontrol_set
	orr	r0, r0, r1
	ldr	lr, Lstart

	.align 5
	@ turn mmu on!
	mov	r0, r0
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c0, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	mov	pc, lr
	nop
	nop
	nop
	nop

	/* NOTREACHED */
Ll1_s_frame:
	.word	L1_S_FRAME

Ltemp_l1_table:
	/* Put the temporary L1 translation table just below the kernel. */
	.word	0x4000

Lstart:
	.word	start
/*
 * Coprocessor register initialization values
 */

	/* bits to set in the Control Register */
Lcontrol_set:
#ifdef ARM11_COMPAT_MMU
#define	CPU_CONTROL_EXTRA	CPU_CONTROL_SYST_ENABLE
#else
#define	CPU_CONTROL_EXTRA	CPU_CONTROL_XP_ENABLE
#endif
	.word CPU_CONTROL_MMU_ENABLE  | \
	      CPU_CONTROL_DC_ENABLE   | \
	      CPU_CONTROL_WBUF_ENABLE |    /* not defined in 1176 (SBO) */ \
	      CPU_CONTROL_32BP_ENABLE |    /* SBO */ \
	      CPU_CONTROL_32BD_ENABLE |    /* SBO */ \
	      CPU_CONTROL_LABT_ENABLE |    /* SBO */ \
		(1 << 16) | 	/* SBO - Global enable for data tcm */ \
		(1 << 18) |	/* SBO - Global enable for insn tcm */ \
	      CPU_CONTROL_UNAL_ENABLE | \
	      CPU_CONTROL_IC_ENABLE | \
	      CPU_CONTROL_EXTRA

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
		(1 << 17) | \
	        (1 << 10)

	/* bits to disable the caches */
Lctl_ID_dis:
	.word	~(CPU_CONTROL_IC_ENABLE|CPU_CONTROL_DC_ENABLE)

/* We'll modify va and pa at run time so we can use relocatable addresses. */
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	va					    ; \
	.word	pa					    ; \
	.word	n_sec					    ; \
	.word	attr					    ;

#ifdef ARM11_COMPAT_MMU
#define	L1_S_APv6_KRW		L1_S_AP_KRW
#else
#define	L1_S_APv6_KRW		L1_S_APv7_KRW
#endif

mmu_init_table:
	/* Add 1MB of VA==PA at 0x00000000 so we can keep the kernel going */
	MMU_INIT(0x0, 0x0,
	    (_end - KERNEL_BASE + 2 * L1_S_SIZE - 1),
	    L1_S_PROTO | L1_S_APv6_KRW)

	MMU_INIT(KERNEL_BASE, 0x0,
	    (_end - KERNEL_BASE + 2 * L1_S_SIZE - 1),
	    L1_S_PROTO | L1_S_APv6_KRW  | L1_S_B | L1_S_C)

	/* Map the 16MB of peripherals */
	MMU_INIT(RPI_KERNEL_IO_VBASE, RPI_KERNEL_IO_PBASE,
	    (RPI_KERNEL_IO_VSIZE + L1_S_SIZE - 1),
	    L1_S_PROTO | L1_S_APv6_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

	.globl	_C_LABEL(rpi_boot_regs)
rpi_boot_regs:
	.space 4 * 4
