/*	$NetBSD: a9_mpsubr.S,v 1.23 2014/09/16 22:29:09 matt Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include <arm/cortex/scu_reg.h>
#include "assym.h"

//#define MPDEBUG

// We'll modify va and pa at run time so we can use relocatable addresses.
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	((va) & 0xffffffff)|(n_sec)		    ; \
	.word	((pa) & 0xffffffff)|(attr)		    ; \

// Set up a preliminary mapping in the MMU to allow us to run at KERNEL_BASE
// with caches on.  If we are MULTIPROCESSOR, save the TTB address.
//
arm_boot_l1pt_init:
#if defined(MULTIPROCESSOR)
	movw	r3, #:lower16:cortex_mmuinfo
	movt	r3, #:upper16:cortex_mmuinfo
#if !defined(KERNEL_BASES_EQUAL)
	sub	r3, r3, #KERNEL_BASE_VOFFSET
#endif
	str	r0, [r3]

	// Make sure the info makes into memory
	mcr	p15, 0, r3, c7, c10, 1		// writeback the cache line
	dsb
#endif

	mov	ip, r1			// save mmu table addr
	// Build page table from scratch
	mov	r1, r0			// Start address to clear memory.
	// Zero the entire table so all virtual addresses are invalid.
	add	r2, r1, #L1_TABLE_SIZE	// Ending address
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
1:	stmia	r1!, {r4-r7}		// 16 bytes at a time
	cmp	r1, r2
	blt	1b

	// Now create our entries per the mmu_init_table.
	l1table	.req r0
	va	.req r1
	pa	.req r2
	n_sec	.req r3
	attr	.req r4
	itable	.req r5

	mov	attr, #0
	mrc	p15, 0, r3, c0, c0, 5	// MPIDR read
	cmp	r3, #0			// not zero?
	movne	attr, #L1_S_V6_S	//    yes, shareable attribute
	mov	itable, ip		// reclaim table address
	b	3f

2:	str	pa, [l1table, va, lsl #2]
	add	va, va, #1
	add	pa, pa, #(L1_S_SIZE)
	subs	n_sec, n_sec, #1
	bhi	2b

3:	ldmia	itable!, {va, pa}
	// Convert va to l1 offset:	va = 4 * (va >> L1_S_SHIFT)
	ubfx	n_sec, va, #0, #L1_S_SHIFT
	lsr	va, va, #L1_S_SHIFT

	// Do we need add sharing for this?
	tst	pa, #(L1_S_C|L1_S_B)	// is this entry cacheable?
	orrne	pa, pa, attr		// add sharing

4:	cmp	n_sec, #0
	bne	2b
	bx	lr			// return

	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	itable
	.unreq	l1table

//
// Coprocessor register initialization values
//
#if defined(CPU_CORTEXA8)
#undef CPU_CONTROL_SWP_ENABLE		// not present on A8
#define CPU_CONTROL_SWP_ENABLE		0
#endif
#ifdef __ARMEL__
#define CPU_CONTROL_EX_BEND_SET		0
#else
#define CPU_CONTROL_EX_BEND_SET		CPU_CONTROL_EX_BEND
#endif
#ifdef ARM32_DISABLE_ALIGNMENT_FAULTS
#define CPU_CONTROL_AFLT_ENABLE_CLR	CPU_CONTROL_AFLT_ENABLE
#define CPU_CONTROL_AFLT_ENABLE_SET	0
#else
#define CPU_CONTROL_AFLT_ENABLE_CLR	0
#define CPU_CONTROL_AFLT_ENABLE_SET	CPU_CONTROL_AFLT_ENABLE
#endif

// bits to set in the Control Register
//
#define CPU_CONTROL_SET \
	(CPU_CONTROL_MMU_ENABLE		|	\
	 CPU_CONTROL_AFLT_ENABLE_SET	|	\
	 CPU_CONTROL_DC_ENABLE		|	\
	 CPU_CONTROL_SWP_ENABLE		|	\
	 CPU_CONTROL_BPRD_ENABLE	|	\
	 CPU_CONTROL_IC_ENABLE		|	\
	 CPU_CONTROL_EX_BEND_SET	|	\
	 CPU_CONTROL_UNAL_ENABLE)

// bits to clear in the Control Register
//
#define CPU_CONTROL_CLR \
	(CPU_CONTROL_AFLT_ENABLE_CLR)

arm_cpuinit:
	// Because the MMU may already be on do a typical sequence to set
	// the Translation Table Base(s).
	mov	ip, lr
	mov	r10, r0			// save TTBR
	mov	r1, #0

	mcr     p15, 0, r1, c7, c5, 0	// invalidate I cache

	mrc	p15, 0, r2, c1, c0, 0	// SCTRL read
	movw	r1, #(CPU_CONTROL_DC_ENABLE|CPU_CONTROL_IC_ENABLE)
	bic	r2, r2, r1		// clear I+D cache enable

#ifdef __ARMEB__
	// SCTRL.EE determines the endianness of translation table lookups.
	// So we need to make sure it's set before starting to use the new
	// translation tables (which are big endian).
	//
	orr	r2, r2, #CPU_CONTROL_EX_BEND
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	pli	[pc, #32]		// preload the next few cachelines
	pli	[pc, #64]
	pli	[pc, #96]
	pli	[pc, #128]
#endif

	mcr	p15, 0, r2, c1, c0, 0	// SCTRL write

	XPUTC(#70)
	dsb				// Drain the write buffers.
1:
	XPUTC(#71)
	mrc	p15, 0, r1, c0, c0, 5	// MPIDR read
	cmp	r1, #0
	orrlt	r10, r10, #0x5b		// MP, cachable (Normal WB)
	orrge	r10, r10, #0x1b		// Non-MP, cacheable, normal WB
	XPUTC(#48)
	mcr	p15, 0, r10, c2, c0, 0	// TTBR0 write
#if defined(ARM_MMU_EXTENDED)
	// When using split TTBRs, we need to set both since the physical
	// addresses we were/are using might be in either.
	XPUTC(#49)
	mcr	p15, 0, r10, c2, c0, 1	// TTBR1 write
#endif

	XPUTC(#72)
#if defined(ARM_MMU_EXTENDED)
	XPUTC(#49)
	mov	r1, #TTBCR_S_N_1	// make sure TTBCR_S_N is 1
#else
	XPUTC(#48)
	mov	r1, #0			// make sure TTBCR is 0
#endif
	mcr	p15, 0, r1, c2, c0, 2	// TTBCR write

	XPUTC(#73)
	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0	// TLBIALL (just this core)

	XPUTC(#74)
	mov	r1, #0			// get KERNEL_PID
	mcr	p15, 0, r1, c13, c0, 1	// CONTEXTIDR write

	// Set the Domain Access register.  Very important!
	XPUTC(#75)
	mov     r1, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r1, c3, c0, 0	// DACR write

	//
	// Enable the MMU, etc.
	//
	XPUTC(#76)
	mrc	p15, 0, r1, c1, c0, 0	// SCTRL read

	movw	r3, #:lower16:CPU_CONTROL_SET
#if (CPU_CONTROL_SET & 0xffff0000)
	movt	r3, #:upper16:CPU_CONTROL_SET
#endif
	orr	r0, r1, r3
#if defined(CPU_CONTROL_CLR) && (CPU_CONTROL_CLR != 0)
	bic	r0, r0, #CPU_CONTROL_CLR
#endif
	//cmp	r0, r1			// any changes to SCTRL?
	//bxeq	ip			//    no, then return.

	pli	1f
	dsb

	// turn mmu on!
	//
	mov	r0, r0			// fetch instruction cacheline
1:	mcr	p15, 0, r0, c1, c0, 0	// SCTRL write

	// Ensure that the coprocessor has finished turning on the MMU.
	//
	mrc	p15, 0, r0, c0, c0, 0	// Read an arbitrary value.
	mov	r0, r0			// Stall until read completes.
	XPUTC(#77)

	bx	ip			// return

	.p2align 2

#if defined(VERBOSE_INIT_ARM) && defined(XPUTC_COM)
#define TIMO		0x25000
#ifndef COM_MULT
#define COM_MULT	1
#endif
xputc:
	mov	r2, #TIMO
#ifdef CONADDR
	movw	r3, #:lower16:CONADDR
	movt	r3, #:upper16:CONADDR
#elif defined(CONSADDR)
	movw	r3, #:lower16:CONSADDR
	movt	r3, #:upper16:CONSADDR
#endif
1:
#if COM_MULT == 1
	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
#else
#if COM_MULT == 2
	ldrh	r1, [r3, #(COM_LSR*COM_MULT)]
#elif COM_MULT == 4
	ldr	r1, [r3, #(COM_LSR*COM_MULT)]
#endif
#ifdef COM_BSWAP
	lsr	r1, r1, #(COM_MULT-1)*8
#endif
#endif
	tst	r1, #LSR_TXRDY
	bne	2f
	subs	r2, r2, #1
	bne	1b
2:
#if COM_MULT == 1
	strb	r0, [r3, #COM_DATA]
#else
#ifdef COM_BSWAP
	lsl	r0, r0, #(COM_MULT-1)*8
#endif
#if COM_MULT == 2
	strh	r0, [r3, #COM_DATA]
#else
	str	r0, [r3, #COM_DATA]
#endif
#endif

	mov	r2, #TIMO
3:
#if COM_MULT == 1
	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
#else
#if COM_MULT == 2
	ldrh	r1, [r3, #(COM_LSR*COM_MULT)]
#elif COM_MULT == 4
	ldr	r1, [r3, #(COM_LSR*COM_MULT)]
#endif
#ifdef COM_BSWAP
	lsr	r1, r1, #(COM_MULT-1)*8
#endif
#endif
	tst	r1, #LSR_TSRE
	bne	4f
	subs	r2, r2, #1
	bne	3b
4:
	bx	lr
#endif /* VERBOSE_INIT_ARM */

//
// Perform the initialization of the Cortex core required by NetBSD.
//
//
cortex_init:
	mov	r10, lr				// save lr

	cpsid	if, #PSR_SVC32_MODE		// SVC32 with no interrupts
        mov	r0, #0
        msr	spsr_sxc, r0			// set SPSR[23:8] to known value

	XPUTC(#64)
#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA15)
	//
	// If SMP is already enabled, don't do anything.
	//
	mrc	p15, 0, r0, c1, c0, 1		// ACTLR read
	tst	r0, #CORTEXA9_AUXCTL_SMP	// test SMP
	bxne	r10				// return if set
#endif

	mrc	p15, 0, r4, c1, c0, 0		// SCTLR read
#if defined(CPU_CORTEXA7)
	//
	// Before turning on SMP, turn off the caches and the MMU.
	//
	dsb
	movw	r1,#(CPU_CONTROL_IC_ENABLE|CPU_CONTROL_DC_ENABLE\
			|CPU_CONTROL_MMU_ENABLE)
	bic	r0, r4, r1			// disable icache/dcache/mmu
	mcr	p15, 0, r0, c1, c0, 0		// SCTLR write
	dsb
	isb
#endif

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		// toss i-cache

#if defined(CPU_CORTEXA5) || defined(CPU_CORTEXA9)
	//
	// Step 1a, invalidate the all cache tags in all ways on the SCU.
	//
	XPUTC(#65)
	mrc	p15, 4, r3, c15, c0, 0		// read cbar
#ifdef __ARMEB__
	setend	le
#endif
	mrc	p15, 0, r0, c0, c0, 5		// MPIDR get
	and	r0, r0, #3			// get our cpu numder
	lsl	r0, r0, #2			// adjust to cpu num shift
	mov	r1, #0xf			// select all ways
	lsl	r1, r1, r0			// shift into place
	str	r1, [r3, #SCU_INV_ALL_REG]	// write scu invalidate all
#ifdef __ARMEB__
	setend	be
#endif
	dsb
	isb
#endif

	//
	// Step 1b, invalidate the data cache
	//
	XPUTC(#66)
#if defined(KERNEL_BASES_EQUAL)
	bl	_C_LABEL(armv7_dcache_wbinv_all)
#else
	movw	ip, #:lower16:_C_LABEL(armv7_dcache_wbinv_all)
	movt	ip, #:upper16:_C_LABEL(armv7_dcache_wbinv_all)
	sub	ip, ip, #KERNEL_BASE_VOFFSET
	blx	ip				// writeback & toss d-cache
#endif
	XPUTC(#67)

	//
	// Check to see if we are really MP before enabling SMP mode
	//
	mrc	p15, 0, r1, c0, c0, 5		// MPIDR get
	ubfx	r1, r1, #30, #2			// get MP bits
	cmp	r1, #2				// is it MP?
	bxne	r10				//    no, return

#ifndef CPU_CORTEXA7
	//
	// Step 2, disable the data cache
	//
	mrc	p15, 0, r2, c1, c0, 0		// SCTLR read
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE	// clear data cache enable
	mcr	p15, 0, r2, c1, c0, 0		// SCTLR write
	isb
	XPUTC(#49)
#endif

#if defined(CPU_CORTEXA5) || defined(CPU_CORTEXA9)
	//
	// Step 3, enable the SCU
	//
	mrc	p15, 4, r3, c15, c0, 0		// read cbar
#ifdef __ARMEB__
	setend	le
#endif
	ldr	r1, [r3, #SCU_CTL]		// read scu control
	orr	r1, r1, #SCU_CTL_SCU_ENA	// set scu enable flag
	str	r1, [r3, #SCU_CTL]		// write scu control
#ifdef __ARMEB__
	setend	be
#endif
	dsb
	isb
	XPUTC(#50)
#endif /* CORTEXA5 || CORTEXA9 */

#ifdef CPU_CORTEXA7
	//
	// The MMU is off.  Make sure the TLB is invalidated before
	// turning on SMP.
	//
	mov	r0, #0
	mcr	p15, 0, r1, c8, c7, 0	// TLBIALL (just this core)
#endif

	// For the A7, SMP must be on ldrex/strex to work.
	//
#if defined(MULTIPROCESSOR) || defined(CPU_CORTEXA7)
#if defined(CPU_CORTEXA5) || defined(CPU_CORTEXA7) || defined(CPU_CORTEXA9)
	//
	// Step 4a, set ACTLR.SMP=1
	//
	mrc	p15, 0, r0, c1, c0, 1		// ACTLR read
	orr	r0, r0, #CORTEXA9_AUXCTL_SMP	// enable SMP

#if defined(CPU_CORTEXA5) || defined(CPU_CORTEXA9)
	//
	// Step 4a (continued on A5/A9), ACTLR.FW=1)
	//
	orr	r0, r0, #CORTEXA9_AUXCTL_FW	// enable cache/tlb/coherency
#endif	/* A5 || A9 */
#if defined(CPU_CORTEXA9)
	//
	// Step 4b (continued on A9), ACTLR.L2PE=1)
	//
	orr	r0, r0, #CORTEXA9_AUXCTL_L2PE	// enable L2 cache prefetch
#endif

	mcr	p15, 0, r0, c1, c0, 1		// ACTLR write
	isb
	dsb
#endif	/* A5 || A7 || A9 */
#endif	/* MULTIPROCESSOR */

	//
	// Step 4b, restore SCTRL (enable the data cache)
	//
	orr	r4, r4, #CPU_CONTROL_IC_ENABLE	// enable icache
	orr	r4, r4, #CPU_CONTROL_DC_ENABLE	// enable dcache
	mcr	p15, 0, r4, c1, c0, 0		// SCTRL write
	isb
	XPUTC(#45)

	bx	r10
ASEND(cortex_init)

#ifdef MULTIPROCESSOR
	.pushsection .data
	.align	2
	.globl	cortex_mmuinfo
	.type	cortex_mmuinfo,%object
cortex_mmuinfo:
	.space	4
//
// If something goes wrong in the inital mpstartup, catch and record it.
//
#ifdef MPDEBUG
	.globl	cortex_mpfault
	.type	cortex_mpfault,%object
cortex_mpfault:
	.space	16		// PC, LR, FSR, FAR
#endif
	.popsection
#endif // MULTIPROCESSOR

// Secondary processors come here after exiting the SKU ROM.
// Switches to kernel's endian almost immediately.
//
cortex_mpstart:
#ifndef MULTIPROCESSOR
	//
	// If not MULTIPROCESSOR, drop CPU into power saving state.
	//
3:	wfi
	b	3b
#else
#ifdef __ARMEB__
	setend	be				// switch to BE now
#endif

#if 0
	mrc	p15, 0, r0, c1, c1, 2		// NSACR read
	// Allow non-secure access to ACTLR[SMP]
	orr	r0, r0, #NSACR_SMP
#ifdef FPU_VFP
	// Allow non-secure access to VFP/Neon
	orr	r0, r0, #NSACR_VFPCP
#endif
	mcr	p15, 0, r0, c1, c1, 2		// NSACR write

	// Allow non-secure access to CPSR[A,F], go to non-secure mode
	mrc	p15, 0, r0, c1, c1, 0		// SCR read
	orr	r0, r0, #0x31
	bic	r0, r4, #0x0e		// non monitor extabt, irq, fiq
	mcr	p15, 0, r0, c1, c1, 0		// SCR write
	isb
#endif

	bl	cortex_init

	// We are in SMP mode now.
	//

	// Get our initial temporary TTB so we can switch to it.
	movw	r7, #:lower16:_C_LABEL(cortex_mmuinfo)
	movt	r7, #:upper16:_C_LABEL(cortex_mmuinfo)
#if !defined(KERNEL_BASES_EQUAL)
	sub	r7, r7, #KERNEL_BASE_VOFFSET
#endif
	dmb
	ldr	r0, [r7]			// load saved TTB address

	// After we turn on the MMU, we will return to do rest of the
	// MP startup code in .text.
	//
	movw	lr, #:lower16:cortex_mpcontinuation
	movt	lr, #:upper16:cortex_mpcontinuation
	b	arm_cpuinit
#endif // MULTIPROCESSOR
ASEND(cortex_mpstart)

#ifdef MULTIPROCESSOR
	.pushsection .text
cortex_mpcontinuation:
#ifdef MPDEBUG
	//
	// Setup VBAR to catch errors
	//
	adr	r2, cortex_mpvector
	mcr	p15, 0, r2, c12, c0, 0		// VBAR set
	isb

	mrc	p15, 0, r0, c1, c0, 0		// SCTRL read
#ifdef MULTIPROCESSOR
	bic	r0, r0, #CPU_CONTROL_VECRELOC	// use VBAR
#endif
	mcr	p15, 0, r0, c1, c0, 0		// SCTRL write
	dsb
	isb
#endif

#ifdef MPDEBUG
	movw	r9, #:lower16:_C_LABEL(arm_cpu_marker)
	movt	r9, #:upper16:_C_LABEL(arm_cpu_marker)
	str	pc, [r9]
	str	r2, [r9, #4]
#endif

	mrc	p15, 0, r4, c0, c0, 5		// MPIDR get
	and	r4, r4, #7			// get our cpu numder
	mov	r5, #1				// make a bitmask of it
	lsl	r5, r5, r4			// shift into position
#ifdef MPDEBUG
	str	pc, [r9]
#endif

	mov	r1, r5
	movw	r0, #:lower16:_C_LABEL(arm_cpu_hatched)
	movt	r0, #:upper16:_C_LABEL(arm_cpu_hatched)
	bl	_C_LABEL(atomic_or_32)		// show we've hatched
	sev

	//
	// Now we wait for cpu_boot_secondary_processors to kick us the
	// first time.  This means the kernel L1PT is ready for us to use.
	//
	movw	r6, #:lower16:_C_LABEL(arm_cpu_mbox)
	movt	r6, #:upper16:_C_LABEL(arm_cpu_mbox)
#ifdef MPDEBUG
	str	pc, [r9]
#endif
3:	dmb					// make stores visible
	ldr	r2, [r6]			// load mbox
	tst	r2, r5				// is our bit set?
#ifdef MPDEBUG
	str	pc, [r9]
	str	r2, [r9, #4]
#endif
	wfeeq					//   no, back to sleep
	beq	3b				//   no, and try again

#ifdef MPDEBUG
	str	pc, [r9]
#endif

	movw	r0, #:lower16:_C_LABEL(kernel_l1pt)
	movt	r0, #:upper16:_C_LABEL(kernel_l1pt)
	ldr	r0, [r0, #PV_PA]		// now get the phys addr
#ifdef MPDEBUG
	str	pc, [r9]
	str	r0, [r9, #4]
#endif
#ifdef ARM_MMU_EXTENDED
	mov	r1, #0
#endif
	bl	_C_LABEL(armv7_setttb)		// set the TTB

	mov	r0, #DOMAIN_DEFAULT
	mcr	p15, 0, r0, c3, c0, 0		// DACR write

	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0		// invalidate the TLB

	mrc	p15, 0, r1, c2, c0, 2		// TTBCR get
	orr	r1, r1, #TTBCR_S_PD0		// prevent lookups via TTBR0
	mcr	p15, 0, r1, c2, c0, 2		// TTBCR set

#ifdef MPDEBUG
	str	pc, [r9]			// we've got this far
	str	r4, [r9, #4]
#endif

	//
	// Tell arm32_kvminit we've load the new TTB
	//
	mov	r0, r6
	mvn	r1, r5				// pass inverted mask to clear
	bl	_C_LABEL(atomic_and_32)
	sev					// wake the master

#ifdef MPDEBUG
	str	pc, [r9]			// we've got this far
#endif

	// Wait for cpu_boot_secondary_processors the second time.
	//
4:	dmb					// data memory barrier
	ldr	r2, [r6]			// load mbox
	tst	r2, r5				// is our bit set?
	wfeeq					//    no, back to waiting
	beq	4b				//    no, and try again

#ifdef MPDEBUG
	str	pc, [r9]			// we've got this far
#endif

	movw	r0, #:lower16:cpu_info
	movt	r0, #:upper16:cpu_info		// get pointer to cpu_infos
	ldr	r5, [r0, r4, lsl #2]		// load our cpu_info
	ldr	r6, [r5, #CI_IDLELWP]		// get the idlelwp
	ldr	r7, [r6, #L_PCB]		// now get its pcb
	ldr	sp, [r7, #PCB_KSP]		// finally, we can load our SP
#ifdef TPIDRPRW_IS_CURCPU
	mcr	p15, 0, r5, c13, c0, 4		// squirrel away curcpu()
#elif defined(TPIDRPRW_IS_CURLWP)
	mcr	p15, 0, r6, c13, c0, 4		// squirrel away curlwp()
#else
#error either TPIDRPRW_IS_CURCPU or TPIDRPRW_IS_CURLWP must be defined
#endif
	str	r6, [r5, #CI_CURLWP]		// and note we are running on it

#ifdef MPDEBUG
	str	pc, [r9]			// r9 still has arm_cpu_marker
#endif

	mov	r0, r5				// pass cpu_info
	mov	r1, r4				// pass cpu_id
	movw	r2, #:lower16:MD_CPU_HATCH	// pass md_cpu_hatch
	movt	r2, #:upper16:MD_CPU_HATCH	// pass md_cpu_hatch
	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(idle_loop)		// never to return
ASEND(cortex_mpcontinuation)

#ifdef MPDEBUG
// Our exception table.  We only care about prefetch/data/address aborts.
//
	.p2align 5
cortex_mpvector:
	b	.	@ reset
	b	.	@ undefined
	b	.	@ swi
	b	xprefetch_abort
	b	xdata_abort
	b	xaddress_abort
	b	.	@ irq
	b	.	@ fiq

xprefetch_abort:
	adr	r10, xprefetch_abort
	mrc	p15, 0, r11, c5, c0, 1		// IFSR
	mrc	p15, 0, r12, c6, c0, 1		// IFAR
	b	xcommon_abort
xdata_abort:
	adr	r10, xdata_abort
	mrc	p15, 0, r11, c5, c0, 0		// DFSR
	mrc	p15, 0, r12, c6, c0, 0		// DFAR
	b	xcommon_abort
xaddress_abort:
	adr	r10, xaddress_abort
	mrc	p15, 0, r11, c5, c0, 0		// DFSR
	mrc	p15, 0, r12, c6, c0, 0		// DFAR
xcommon_abort:
	movw	r8, #:lower16:cortex_mpfault	// where we should be
	movt	r8, #:upper16:cortex_mpfault	// where we should be
	stmia	r8, {r10-r12,lr}		// save type, PC, FSR, FAR
	b	.				// loop forever
#endif
	.popsection
#endif // MULTIPROCESSOR
