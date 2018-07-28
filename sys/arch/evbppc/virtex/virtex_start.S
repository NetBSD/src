/*	$NetBSD: virtex_start.S,v 1.8.52.1 2018/07/28 04:37:33 pgoyette Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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
 * This file is based on startup code of Walnut and Explora boards.
 */

#include "opt_ddb.h"
#include "opt_lockdebug.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "opt_ppcparam.h"
#include "opt_virtex.h"
#include "assym.h"
#include "ksyms.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/dcr4xx.h>


/* N megabytes. */
#define MB(n) 				((n)*1024*1024)

/* Set bit (beginning with MSB) for each 128MB of RAM. */
#define PHYSMEM_REGIONS_MASK 		~((1 << (32 - MB(PHYSMEM)/MB(128))) - 1)

/* For kvm_mkdb, supposed to mark the start of kernel text. */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/* Startup entry. This must be the first thing in the text segment! */
	.text
	.globl	__start
__start:
	/* Disable MMU/exceptions */
	lis	%r0, 0
	mtmsr	%r0

	/* Disable timers */
	lis 	%r0, 0
	mttcr 	%r0

	sync
	isync

	/* Disable caches */
	mtdccr 	%r0
	mticcr	%r0
	sync
	isync

	/* Invalidate I$, operands ignored on the 405 */
	li 	%r0,0 			/* just in case... */
	iccci 	%r0,%r0

	/* Invalidate D$, hardcoded for 16KB size, 32B line */
	li      %r7,256 		/* # of congruence classes */
	mtctr   %r7
	li      %r6,0
1:
	dccci   %r0,%r6 		/* invalidates both ways */
	addi    %r6,%r6,32
	bdnz    1b

        /*
         * Errata 213: 	Incorrect data may be flushed from the data cache.
         * Cores: 	PPC405D5X1, PPC405D5X2
         * Workaround: 	#1, CCR0 modification sequence #2
         * Note: 	Meaning of bits we need to set is undocumented.
         */
	sync
        mfccr0  %r0
        oris    %r0,%r0,0x50000000@h
        mtccr0  %r0
	isync

	/*
	 * Errata  58: 	Load string instructions may write incorrect
	 * 		data into the last GPR targeted in operation.
	 * Cores: 	PPC405GP
	 * Workaround: 	set OCM0_DSCNTL[DSEN]=0 and OCM0_DSCNTL[DOF]=0
	 */
	mtdcr   DCR_OCM0_DSCNTL,%r0    /* Disable Data access to OCM */

#if 0
	/* Allow cacheing for whole RAM. */
	lis	%r0,PHYSMEM_REGIONS_MASK@ha
	ori 	%r0,%r0,PHYSMEM_REGIONS_MASK@l
#else
#ifndef PPC_4XX_NOCACHE
	/* Allow cacheing for only the first 1GB of RAM */
	lis 	%r0,0xff00
	mtdccr	%r0
	mticcr	%r0
#endif /* PPC_4XX_NOCACHE */
#endif

	/* Invalidate all TLB entries */
	tlbia
	sync
	isync

	/* Set kernel MMU context, we'll enable MMU in initppc() */
	li	%r0,KERNEL_PID
	mtpid	%r0
	sync
	isync

	/* Setup endkernel argument for initppc() and INIT_CPUINFO */
	lis	%r4,_C_LABEL(end)@h
	ori 	%r4,%r4,_C_LABEL(end)@l

	/* Clear .bss segment */
	lis 	%r7,_C_LABEL(edata)-4@h
	ori 	%r7,%r7,_C_LABEL(edata)-4@l
	li 	%r3,0
2: 	stwu    %r3,4(%r7)
	cmpw    %r7,%r4
	bne+    2b

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* We don't have a symbol table, so set startsym = endsym = end */
	lis	%r7,_C_LABEL(startsym)@ha
	ori 	%r7,%r7,_C_LABEL(startsym)@l
	stw	%r4,0(%r7)
	lis	%r7,_C_LABEL(endsym)@ha
	ori 	%r7,%r7,_C_LABEL(endsym)@l
	stw	%r4,0(%r7)
#endif

	/* INIT_CPUINFO will 'addi', so clean up. */
	lis 	%r1,0

	INIT_CPUINFO(4,1,9,0)

	/* startkernel argument for initppc */
	lis	%r3,__start@h
	addi	%r3,%r3,__start@l

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop: 	/* UNREACHED */
	b	loop

#include <powerpc/ibm4xx/4xx_locore.S>
