/*	$NetBSD: locore.S,v 1.11.8.5 2001/03/12 13:29:11 bouyer Exp $	*/

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
#include "opt_multiprocessor.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

#define	INTSTK	(8*1024)	/* 8K interrupt stack */
#define	SPILLSTK 1024		/* 1K spill stack */

/*
 * Globals
 */
GLOBAL(esym)
	.long	0			/* end of symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

/*
 * File-scope for locore.S
 */
	.data
idle_u:
	.long	0			/* fake uarea during idle after exit */
openfirmware_entry:
	.long	0			/* openfirmware entry point */

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
	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#ifdef DDB
	/*
	 * XXX Account for symbols.
	 */
#endif
	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)
	addi	8,8,NBPG
	lis	9,idle_u@ha
	stw	8,idle_u@l(9)
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

	li	0,0			/* turn off any ints/mmu/etc. */
	mtmsr	0
	isync

	blrl				/* call OpenFirmware */

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

/*
 * Pull in common switch / setfault code.
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Child comes here at the end of a fork.
 * Mostly similar to the above.
 */
	.globl	_C_LABEL(fork_trampoline)
_C_LABEL(fork_trampoline):
	xor	3,3,3
	lis	4,_C_LABEL(machine_interface)+SPLX@ha
	lwz	0,_C_LABEL(machine_interface)+SPLX@l(4)
	mtlr	0
	blrl
	mtlr	31
	mr	3,30
	blrl				/* jump indirect to r31 */
	b	trapexit

/*
 * Pull in common trap vector code.
 */
#include <powerpc/powerpc/trap_subr.S>
