/*	$NetBSD: locore.S,v 1.41.8.4 2002/02/28 04:10:40 nathanw Exp $	*/

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
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>
#ifdef NEWPMAP
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

GLOBAL(ofmsr)
	.long	0,0,0,0,0		/* msr & sprg[0-3] used in Open Firmware */

/*
 * File-scope for locore.S
 */
#if !defined(MULTIPROCESSOR)
idle_u:
	.long	0			/* fake uarea during idle after exit */
#endif
openfirmware_entry:
	.long	0			/* openfirmware entry point */
srsave:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

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
#ifdef	FIRMWORKSBUGS
	mfmsr	0
	andi.	0,0,PSL_IR|PSL_DR
	beq	1f

	bl	_C_LABEL(ofwr_init)
1:
#endif
	mfmsr	0
	lis	9,ofmsr@ha
	stw	0,ofmsr@l(9)
        mfsprg  0,0
       	stw	0,ofmsr+4@l(9)
        mfsprg  0,1
       	stw	0,ofmsr+8@l(9)
        mfsprg  0,2
       	stw	0,ofmsr+12@l(9)
        mfsprg  0,3
       	stw	0,ofmsr+16@l(9)

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
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)
	addi	8,8,NBPG
#if defined(MULTIPROCESSOR)
	lis	9,_C_LABEL(cpu_info)@ha
	addi	9,9,_C_LABEL(cpu_info)@l
	addi	8,8,INTSTK
	stw	8,CI_INTSTK(9)
	addi	8,8,SPILLSTK
	stw	8,CI_SPILLSTK(9)
	stw	8,CI_IDLE_PCB(9)
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

	lis	8,openfirmware_entry@ha
	stw	5,openfirmware_entry@l(8) /* save client interface handler */
	lis	3,__start@ha
	addi	3,3,__start@l
	mr	5,6			/* args string */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)
	b	_C_LABEL(OF_exit)

/*
 * OpenFirmware entry point
 */
ENTRY(openfirmware)
	mflr	0			/* save return address */
	stw	0,4(1)
	stwu	1,-16(1)		/* setup stack frame */

	mfmsr	4			/* save msr */
	stw	4,8(1)

	lis	4,openfirmware_entry@ha	/* get firmware entry point */
	lwz	4,openfirmware_entry@l(4)
	mtlr	4

	li	0,0			/* clear battable translations */
	mtdbatu	2,0
	mtdbatu	3,0
	mtibatu	2,0
	mtibatu	3,0

	lis	4,srsave@ha		/* save old SR */
	addi	4,4,srsave@l
	li	5,0
1:	mfsrin	0,5
	stw	0,0(4)
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

	lis	4,_C_LABEL(ofw_pmap)@ha	/* load OFW SR */
	addi	4,4,_C_LABEL(ofw_pmap)@l
	lwz	0,PM_KERNELSR(4)
	cmpwi	0,0			/* pm_sr[KERNEL_SR] == 0? */
	beq	2f			/* then skip (not initialized yet) */
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b
2:
	lis	4,ofmsr@ha		/* Open Firmware msr + sprg[0-3] */
	lwz	5,ofmsr+4@l(4)
	mtsprg	0,5
	lwz	5,ofmsr+8@l(4)
	mtsprg	1,5
	lwz	5,ofmsr+12@l(4)
	mtsprg	2,5
	lwz	5,ofmsr+16@l(4)
	mtsprg	3,5
	lwz	5,ofmsr@l(4)
	mtmsr	5
	isync

	blrl				/* call Open Firmware */

	lis	4,srsave@ha		/* restore saved SR */
	addi	4,4,srsave@l
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

	lwz	4,8(1)			/* restore msr */
	mtmsr	4
	isync

	lwz	1,0(1)			/* and return */
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * Switch to/from OpenFirmware real mode stack
 *
 * Note: has to be called as the very first thing in OpenFirmware interface
 * routines.
 * E.g.:
 * int
 * OF_xxx(arg1, arg2)
 * type arg1, arg2;
 * {
 *	static struct {
 *		char *name;
 *		int nargs;
 *		int nreturns;
 *		char *method;
 *		int arg1;
 *		int arg2;
 *		int ret;
 *	} args = {
 *		"xxx",
 *		2,
 *		1,
 *	};
 *
 *	ofw_stack();
 *	args.arg1 = arg1;
 *	args.arg2 = arg2;
 *	if (openfirmware(&args) < 0)
 *		return -1;
 *	return args.ret;
 * }
 */

	.local	firmstk
	.comm	firmstk,NBPG,8

ENTRY(ofw_stack)
	mfmsr	8			/* turn off interrupts */
	andi.	0,8,~(PSL_EE|PSL_RI)@l
	mtmsr	0
	stw	8,4(1)			/* abuse return address slot */

	lwz	5,0(1)			/* get length of stack frame */
	subf	5,1,5

	lis	7,firmstk+NBPG-8@ha
	addi	7,7,firmstk+NBPG-8@l
	lis	6,ofw_back@ha
	addi	6,6,ofw_back@l
	subf	4,5,7			/* make room for stack frame on
					   new stack */
	stw	6,-4(7)			/* setup return pointer */
	stwu	1,-8(7)

	stw	7,-8(4)

	addi	3,1,8
	addi	1,4,-8
	subi	5,5,8

	b	_C_LABEL(ofbcopy)	/* and copy it */

ofw_back:
	lwz	1,0(1)			/* get callers original stack pointer */

	lwz	0,4(1)			/* get saved msr from abused slot */
	mtmsr	0

	lwz	1,0(1)			/* return */
	lwz	0,4(1)
	mtlr	0
	blr

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
