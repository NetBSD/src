/*	$NetBSD: mpc85xx_start.S,v 1.6.12.1 2014/08/10 06:53:57 tls Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>
#include <powerpc/asm.h>

RCSID("$NetBSD: mpc85xx_start.S,v 1.6.12.1 2014/08/10 06:53:57 tls Exp $")

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "opt_ppcparam.h"

#include "ksyms.h"

#include <sys/param.h>

#include <powerpc/spr.h>
#include <powerpc/trap.h>
#include <powerpc/psl.h>
#include <powerpc/booke/pte.h>
#include <powerpc/booke/spr.h>
#define	LBC_PRIVATE
#include <powerpc/booke/e500reg.h>

#include "assym.h"

#define	INTSTK	0

	.machine e500
/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):
        .globl  __start 
__start:

/*
 * Startup entry.  Note, this must be the first thing in the text segment!
 */
	mr	%r8,%r6		/* cmdline (char *) */
	mr	%r7,%r5		/* consdev (char *) */
	mr	%r6,%r4		/* os_hdr * */
	mr	%r5,%r3		/* board info * */
#ifdef DEBUG
	/*
	 * Set all the registers we don't care about to a known junk value.
	 */
	lis	%r2,0xdeadbeef@h
	ori	%r2,%r2,0xdeadbeef@l
	mr	%r9,%r2
	mr	%r10,%r9
	mr	%r11,%r9
	mr	%r12,%r9
	mr	%r13,%r9
	mr	%r14,%r9
	mr	%r15,%r9
	mr	%r16,%r9
	mr	%r17,%r9
	mr	%r18,%r9
	mr	%r19,%r9
	mr	%r20,%r9
	mr	%r21,%r9
	mr	%r22,%r9
	mr	%r23,%r9
	mr	%r24,%r9
	mr	%r25,%r9
	mr	%r26,%r9
	mr	%r27,%r9
	mr	%r28,%r9
	mr	%r29,%r9
	mr	%r30,%r9
	mr	%r31,%r9
#endif /* DEBUG */

	li	%r0,0
	mtmsr	%r0			/* Disable FPU/MMU/exceptions */
	isync

/* get start of bss */
	lis	%r15,_C_LABEL(_edata)-4@ha
	addi	%r15,%r15,_C_LABEL(_edata)-4@l
/* get end of kernel memory */
	lis	%r16,_C_LABEL(end)@ha
	addi	%r16,%r16,_C_LABEL(end)@l
/* zero bss */
	sub	%r17,%r16,%r15
	addi	%r17,%r17,3+USPACE
	rlwinm	%r3,%r17,32-2,2,31	/* srwl	%r3,%r17,2 */
	mtctr	%r3
	li	%r0,0
2:	stwu	%r0,4(%r15)
	bdnz	2b

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* If we had symbol table location we'd store it here and would've adjusted r8 here */
	lis	%r17,_C_LABEL(startsym)@ha
	stw	%r18,_C_LABEL(startsym)@l(%r17)
	lis	%r17,_C_LABEL(endsym)@ha
	stw	%r18,_C_LABEL(endsym)@l(%r17)
#endif

	/* Set kernel MMU context. */
	li	%r0,KERNEL_PID
	mtpid	%r0
	isync

	INIT_CPUINFO(16,1,18,17)	/* r16 has &_end */
	mr	%r4,%r16		/* remember kernelend */
	mtsprg2	%r13			/* r13 has &lwp0, put into sprg2 */
	GET_CPUINFO(%r17)
	addi	%r17,%r17,CI_SAVELIFO
	mtsprg3 %r17
	mr	%r18,%r31		/* make deadbeef again */
	mr	%r17,%r31		/* make deadbeef again */
	mr	%r16,%r31		/* make deadbeef again */
	mr	%r15,%r31		/* make deadbeef again */

#if defined(GXEMUL)
	/*
	 * This is used to step through the external interrupt vector
	 * to validate it.
	 */
	lis	%r29,3f@ha
	addi	%r29,%r29,3f@l
	mtsrr0	%r29
	mfmsr	%r0
	mtsrr1	%r0
	lis	%r31,0xdeadf231@ha
	addi	%r31,%r31,0xdeadf231@l
	mtlr	%r31
	addi	%r31,%r31,-0x10
	mtcr	%r31
	addi	%r31,%r31,-0x10
	mtctr	%r31
	addi	%r31,%r31,-0x10
	mtxer	%r31
	addi	%r31,%r31,-0x10
	addi	%r30,%r31,-0x10
	addi	%r29,%r30,-0x10
	addi	%r28,%r29,-0x10
	addi	%r27,%r28,-0x10
	addi	%r26,%r27,-0x10
	addi	%r25,%r26,-0x10
	addi	%r24,%r25,-0x10
	addi	%r23,%r24,-0x10
	addi	%r22,%r23,-0x10
	addi	%r21,%r22,-0x10
	addi	%r20,%r21,-0x10
	addi	%r19,%r20,-0x10
	addi	%r18,%r19,-0x10
	addi	%r17,%r18,-0x10
	addi	%r16,%r17,-0x10
	addi	%r15,%r16,-0x10
	addi	%r14,%r15,-0x10
	addi	%r13,%r14,-0x10
	addi	%r12,%r13,-0x10
	addi	%r11,%r12,-0x10
	addi	%r10,%r11,-0x10
	addi	%r9,%r10,-0x10
	addi	%r8,%r9,-0x10
	addi	%r7,%r8,-0x10
	addi	%r6,%r7,-0x10
	addi	%r5,%r6,-0x10
	addi	%r4,%r5,-0x10
	addi	%r3,%r4,-0x10
	addi	%r2,%r3,-0x10
	/* leave r1 alone */
	addi	%r0,%r2,-0x20
	b	_C_LABEL(instruction_tlb_error_vector)
	//b	_C_LABEL(program_vector)
	//b	_C_LABEL(external_input_vector)
3:
#endif

	/*
	 * TB is 50Mhz, watchdog should be ~10 seconds which makes that
	 * 500 million or 0x20000000.  Since it takes 3 bit transitions
	 * we really want 0x10000000.   That's bit 63-28 or 35.  This
	 * means we want WPEXT,WP to be 0b10_0011.
	 */
	lis	%r3,(TCR_WRC_RESET|TCR_WP_2_N(0x23))@ha
	addi	%r3,%r3,(TCR_WRC_RESET|TCR_WP_2_N(0x23))@l
	mtspr	SPR_TCR, %r3
	li	%r0, 0
	mtspr	SPR_TBL, %r0
	mtspr	SPR_TBU, %r0
	mtspr	SPR_DBCR0, %r0

#if 0
	/*
	 * Force all dirty lines in the kernel area to memory.
	 */
	lis	%r9,kernel_text@ha
	addi	%r9,%r9,kernel_text@l
4:	dcbst	%r0,%r9
	addi	%r9,%r9,32
	cmplw	%r9,%r4
	blt	%cr0,4b
	mbar	1
	msync

	/*
	 * Turn off the data cache, and then invalidate it.
	 */
	li	%r3, 0
	mtspr	SPR_L1CSR0, %r3
	li	%r3, L1CSR_CFI
	mtspr	SPR_L1CSR0, %r3
#endif

#if 1
	/*
	 * Clear any locks from the data or instruction caches.
	 */
	mfspr	%r3, SPR_L1CSR0
	ori	%r3, %r3, L1CSR_CLFR
	mtspr	SPR_L1CSR0, %r3
	mfspr	%r3, SPR_L1CSR1
	ori	%r3, %r3, L1CSR_CLFR
	mtspr	SPR_L1CSR1, %r3
#endif

	lis	%r3,__start@ha
	addi	%r3,%r3,__start@l

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */

#include <powerpc/booke/trap_subr.S>
#include <powerpc/powerpc/locore_subr.S>
#include <powerpc/powerpc/pio_subr.S>
#ifdef PPC_HAVE_SPE
#include <powerpc/booke/spe_subr.S>
#endif
#if defined(MULTIPROCESSOR)
#include <powerpc/booke/e500_mpsubr.S>
#endif

#if 0
	.p2align 5
ENTRY(tlbwe)
	isync
	tlbwe
	isync
	blr
#endif
