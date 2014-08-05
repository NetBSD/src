/*	$NetBSD: obs600_locore.S,v 1.6 2014/08/05 16:28:39 joerg Exp $	*/

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

#undef PPC_4XX_NOCACHE

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

/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Kernel start routine for OpenBlockS600
 * this code is excuted at the very first after the kernel is loaded
 * by U-Boot.
 */
	.text
	.globl	__start
__start:
	mr	%r31, %r3		/* argc of 'go's */
	mr	%r30, %r4		/* argv of 'go's */
	mr	%r29, %r6		/* arg string of 'bootm's */

	li	%r0, 0
	mtmsr	%r0			/* Disable FPU/MMU/exceptions */
	isync

#ifdef PPC_4XX_NOCACHE
	/* Disable all caches for physical addresses */
	li	%r0, 0
#else
	/* Allow cacheing for only the first 2GB of RAM */
	lis	%r0, 0xffff
#endif
	mtdccr	%r0
	mticcr	%r0

	/* Invalidate all TLB entries */
	tlbia
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
2:	stwu	%r4, 4(%r3)
	cmpw	%r3, %r8
	bne+	2b

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
	mtpid	%r0
	sync

	INIT_CPUINFO(8,1,9,0)
	mr	%r4, %r8

	lis	%r3, __start@ha
	addi	%r3, %r3, __start@l

	mr	%r5, %r31		/* argc of 'go's */
	mr	%r6, %r30		/* argv of 'go's */
	mr	%r7, %r29		/* arg strings of 'bootm's */

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

not_reached:
	b	not_reached

#include <powerpc/ibm4xx/4xx_locore.S>
