/*	$NetBSD: locore.S,v 1.47 2002/07/28 07:07:00 chs Exp $	*/

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
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>
#ifndef OLDPMAP
#include <machine/vmparam.h>
#endif

#include <powerpc/spr.h>
/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

#if defined(MULTIPROCESSOR)
/*
 * Get varios per-cpu values.
 */
#if 1
#define GET_CPUINFO(r)	\
	mfsprg	r,0
#else
#define	GET_CPUINFO(r)						\
	mfspr	r,SPR_PIR;		/* r = cpu_number() */	\
	mulli	r,r,CI_SIZE;					\
	addis	r,r,_C_LABEL(cpu_info)@ha;			\
	addi	r,r,_C_LABEL(cpu_info)@l;
#endif
#endif

#define	INTSTK   8192			/* 8K interrupt stack */
#define	SPILLSTK 4096			/* 4K spill stack */

/*
 * Globals
 */
	.data
GLOBAL(esym)
	.long	0			/* end of symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

GLOBAL(intrnames)
	.asciz	"irq0", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
	.asciz	"irq16", "irq17", "irq18", "irq19"
	.asciz	"irq20", "irq21", "irq22", "irq23"
	.asciz	"irq24", "irq25", "irq26", "irq27"
	.asciz	"irq28", "irq29", "irq30", "irq31"
	.asciz	"irq32", "irq33", "irq34", "irq35"
	.asciz	"irq36", "irq37", "irq38", "irq39"
	.asciz	"irq40", "irq41", "irq42", "irq43"
	.asciz	"irq44", "irq45", "irq46", "irq47"
	.asciz	"irq48", "irq49", "irq50", "irq51"
	.asciz	"irq52", "irq53", "irq54", "irq55"
	.asciz	"irq56", "irq57", "irq58", "irq59"
	.asciz	"irq60", "irq61", "irq62", "irq63"
	.asciz	"clock", "softclock", "softnet", "softserial"
GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0
GLOBAL(eintrcnt)

/*
 * File-scope for locore.S
 */
#if !defined(MULTIPROCESSOR)
idle_u:
	.long	0			/* fake uarea during idle after exit */
#endif
OF_buffer:
	.space	NBPG

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
	bl	_C_LABEL(ofwinit)	/* init OF */

	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#ifdef DDB
	/* skip symbol table */
	cmpwi	6,0
	beq	1f
	add	9,6,7			/* r9 = args + l */
	lwz	9,-8(9)			/* esym */
	cmpwi	9,0
	beq	1f
	mr	8,9
1:
#endif
	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9
#if defined(MULTIPROCESSOR)
	lis	9,_C_LABEL(cpu_info)@ha
	addi	9,9,_C_LABEL(cpu_info)@l
	addi	8,8,INTSTK
	stw	8,CI_INTSTK(9)
	addi	8,8,SPILLSTK
	stw	8,CI_SPILLSTK(9)
	stw	8,CI_IDLE_PCB(9)
	stw	0,USPACE-16(8)		/* terminate idle stack chain */
#else
	lis	9,idle_u@ha
	stw	8,idle_u@l(9)
#endif
	addi	8,8,USPACE		/* space for idle_u */
	lis	9,_C_LABEL(proc0paddr)@ha
	stw	8,_C_LABEL(proc0paddr)@l(9)
	addi	1,8,USPACE-FRAMELEN	/* stackpointer for proc0 */
	mr	4,1			/* end of mem reserved for kernel */
	xor	0,0,0
	stwu	0,-16(1)		/* end of stack chain */

	lis	8,OF_buffer@ha
	addi	8,8,OF_buffer@l
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)

	lis	3,__start@ha
	addi	3,3,__start@l
	mr	5,6			/* args string */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)
	b	_C_LABEL(OF_exit)

#if defined(MULTIPROCESSOR)
	.globl	_C_LABEL(cpu_spinup_trampoline)
_C_LABEL(cpu_spinup_trampoline):
	li	0,0
	mtmsr	0
	isync

	lis	3,_C_LABEL(cpu_hatch_stack)@ha
	lwz	1,_C_LABEL(cpu_hatch_stack)@l(3)

	bl	_C_LABEL(cpu_hatch)
	bl      _C_LABEL(sched_lock_idle)
	li      30,0
	b       _ASM_LABEL(Idle)
#endif

/*
 * Pull in common Open Firmware code.
 */
#include <powerpc/mpc6xx/ofw_subr.S>

/*
 * Pull in common switch and setfault code.
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Pull in common trap vector code.
 */
#if defined(MULTIPROCESSOR)
#include <powerpc/powerpc/trap_subr_mp.S>
#else
#include <powerpc/powerpc/trap_subr.S>
#endif
