/*	$NetBSD: locore.S,v 1.48.12.1 2017/12/03 11:36:34 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcparam.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>

#include "ksyms.h"

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

/*
 * Globals
 */
	.data
GLOBAL(esym)
	.long	0			/* end of symbol table */
GLOBAL(oeacpufeat)
	.long	0			/* cpu features */

/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Startup entry.  Note, this must be the first thing in the text
 * segment!
 */
	.text
	.globl	__start
__start:
	mr	%r13,%r6
	mr	%r14,%r7
	bl	_C_LABEL(ofwinit)	/* init our OF hooks */

	li	%r0,0
	mtmsr	%r0			/* Disable FPU/MMU/exceptions */

	bl	_C_LABEL(cpu_model_init)
	lis	%r12,oeacpufeat@ha
	lwz	%r12,oeacpufeat@l(%r12)
	andi.	%r12,%r12,OEACPU_64_BRIDGE	/* Compare against OEACPU_64_BRIDGE */
	beq	2f			/* skip if this is a 32bit part */
	li	%r0,0
	sync
	slbia				/* invalidate the slb table */
	sync
	clrldi	%r0,%r0,32		/* clear SF/ISF (high order bits) */
	mtmsrd	%r0
	mtspr	0x118,0		/* clear ASR[V] to enable segregs */
2:
	isync
	/* compute end of kernel memory */
        lis	%r4,_C_LABEL(end)@ha
        addi	%r4,%r4,_C_LABEL(end)@l
#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* skip symbol table */
	mr	%r6,%r13
	mr	%r7,%r14
	cmpwi	%r6,0
	beq	1f
	add	%r9,%r6,%r7			/* r9 = args + l */
	lwz	%r9,-8(%r9)			/* esym */
	cmpwi	%r9,0
	beq	1f
	mr	%r4,%r9
1:
#endif


	/*
	 * Initialize cpu_info[0]
	 */
	INIT_CPUINFO(%r4,%r1,%r9,%r0)

	lis	%r3,__start@ha
	addi	%r3,%r3,__start@l
	mr	%r5,%r6			/* args string */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)
	b	_C_LABEL(OF_exit)

/*
 * Pull in common OFW code.
 */
#include <powerpc/oea/ofw_subr.S>

/*
 * Pull in common switch / setfault code.
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Pull in common trap vector code.
 */
#include <powerpc/powerpc/trap_subr.S>

/*
 * Pull in common pio / bus_space code.
 */
#include <powerpc/powerpc/pio_subr.S>
