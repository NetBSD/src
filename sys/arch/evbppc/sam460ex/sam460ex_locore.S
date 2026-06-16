/*	$NetBSD: sam460ex_locore.S,v 1.1 2026/06/16 21:51:20 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2009 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Kernel entry for the ACube Sam460ex (AMCC 460EX)
 *
 * Work with U-Boot (bootm) or qemu-system-ppc -M sam460ex -kernel
 *
 *	r3 = physical address of the flattened device tree
 *	r6 = ePAPR magic (0x45504150)
 *
 * Boot TLB layout (TID=0, matches any PID):
 *	slot 0: 256MB  VA 0x00000000 -> PA 0x0_00000000, cached  (TS=0)
 *	slot 1:  16MB  VA 0xef000000 -> PA 0x4_ef000000, I+G     (TS=0)
 *
 * TS=1 (KERNEL_PID) entries are pinned later by initppc() via
 * ppc44x_tlb_reserve().
 */

#include "opt_ddb.h"
#include "opt_ppcarch.h"
#include "opt_ppcparam.h"
#include "assym.h"
#include "ksyms.h"

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/tlb.h>

	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

	.text
	.globl	__start
__start:
	mr	%r25, %r3		/* FDT physical address */
	mr	%r24, %r6		/* ePAPR magic */

	li	%r0, 0
	mtmsr	%r0			/* EE/CE/ME off, TS=0 */
	isync

	/* Invalidate L1 caches (whole-cache forms on 440) */
	iccci	%r0, %r0
	dccci	%r0, %r0
	sync
	isync

	/* TLB writes below tag entries with MMUCR[STID]; want TID=0 */
	li	%r0, 0
	mtspr	SPR_MMUCR, %r0
	isync

	/*
	 * Temporary 16MB identity entry at EPN 0 in TS=1
	 */
	li	%r3, 0x0370		/* w0: EPN 0 | V | TS | 16MB */
	li	%r4, 0			/* w1: RPN 0, ERPN 0 */
	li	%r5, 0x0007		/* w2: SX | SW | SR, cached */
	li	%r6, 63
	tlbwe	%r5, %r6, 2
	tlbwe	%r4, %r6, 1
	tlbwe	%r3, %r6, 0
	isync

	lis	%r3, 1f@ha
	addi	%r3, %r3, 1f@l
	mtsrr0	%r3
	li	%r4, (PSL_IS | PSL_DS)@l
	mtsrr1	%r4
	rfi

1:	/* Running in TS=1. Invalidate every TLB entry except slot 63. */
	li	%r3, 0
	li	%r4, 0
2:	cmpwi	%r4, 63
	beq	3f
	tlbwe	%r3, %r4, 0
3:	addi	%r4, %r4, 1
	cmpwi	%r4, 64
	blt	2b
	sync
	isync

	/* pinned TS=0 entries. */

	/* slot 0: 256MB RAM identity, cached */
	li	%r6, 0
	li	%r3, 0x0290		/* w0: EPN 0 | V | 256MB, TS=0 */
	li	%r4, 0			/* w1: RPN 0, ERPN 0 */
	li	%r5, 0x0007		/* w2: SX | SW | SR */
	tlbwe	%r5, %r6, 2
	tlbwe	%r4, %r6, 1
	tlbwe	%r3, %r6, 0

	/* slot 1: 16MB I/O at VA 0xef000000 -> PA 0x4ef000000 */
	li	%r6, 1
	lis	%r3, 0xef00
	ori	%r3, %r3, 0x0270	/* w0: EPN | V | 16MB, TS=0 */
	lis	%r4, 0xef00
	ori	%r4, %r4, 0x0004	/* w1: RPN | ERPN=4 */
	li	%r5, 0x0503		/* w2: I | G | SW | SR */
	tlbwe	%r5, %r6, 2
	tlbwe	%r4, %r6, 1
	tlbwe	%r3, %r6, 0
	isync

	/* back to TS=0, drop the temporary entry. */
	lis	%r3, 4f@ha
	addi	%r3, %r3, 4f@l
	mtsrr0	%r3
	li	%r4, 0
	mtsrr1	%r4
	rfi

4:	li	%r3, 0
	li	%r4, 63
	tlbwe	%r3, %r4, 0
	sync
	isync

	/* get start of bss */
	lis	%r3, _C_LABEL(_edata)-4@ha
	addi	%r3, %r3, _C_LABEL(_edata)-4@l
	/* get end of kernel memory */
	lis	%r8, _C_LABEL(end)@ha
	addi	%r8, %r8, _C_LABEL(end)@l
	/* zero bss */
	li	%r4, 0
5:	stwu	%r4, 4(%r3)
	cmpw	%r3, %r8
	bne+	5b

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* If we had symbol table location we'd store it here and would've adjusted r8 here */
	lis	%r7, _C_LABEL(startsym)@ha
	addi	%r7, %r7, _C_LABEL(startsym)@l
	stw	%r8, 0(%r7)
	lis	%r7, _C_LABEL(endsym)@ha
	addi	%r7, %r7, _C_LABEL(endsym)@l
	stw	%r8, 0(%r7)
#endif

	/* Set kernel MMU context. */
	li	%r0, KERNEL_PID
	mtspr	SPR_PID, %r0
	sync

	INIT_CPUINFO(8,1,9,0)
	mr	%r4, %r8

	lis	%r3, __start@ha
	addi	%r3, %r3, __start@l

	mr	%r5, %r25		/* FDT physical address */
	mr	%r6, %r24		/* ePAPR magic */

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

not_reached:
	b	not_reached

#include <powerpc/ibm4xx/4xx_locore.S>
