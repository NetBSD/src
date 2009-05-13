/*	$NetBSD: locore.S,v 1.17.4.1 2009/05/13 17:16:33 jym Exp $	*/
/*	$OpenBSD: locore.S,v 1.4 1997/01/26 09:06:38 rahnds Exp $	*/

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
#include "fs_kernfs.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcparam.h"
#include "assym.h"
#include "ksyms.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#define _NOREGNAMES
#include <machine/asm.h>

#include <powerpc/spr.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

/*
 * Globals
 */
GLOBAL(startsym)
	.long	0			/* start symbol table */
GLOBAL(endsym)
	.long	0			/* end symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */
GLOBAL(oeacpufeat)
	.long	0				/* cpu features */

/* XXX All of this should die */
GLOBAL(intrnames)
	.asciz	"clock", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
	.asciz	"irq16", "irq17", "irq18", "irq19"
	.asciz	"irq20", "irq21", "irq22", "irq23"
	.asciz	"irq24", "irq25", "irq26", "irq27"
	.asciz	"irq28", "softnet", "softclock", "softserial"
GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)

/*
 * File-scope for locore.S
 */
	.data
idle_u:
	.long	0			/* fake uarea during idle after exit */

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
	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

/*
 * CPU detect.
 * 
 */
	lis	8, 0x7FFF
	ori	8, 8, 0xF3F0
	lwz	9, 0(8)		/* read processor ID */
	andis.	9, 9, 0x0200	/* bit 6: */
	cmpwi	0, 9, 0		/* 0 if read by CPU 0, 1 if read by CPU 1 */
	bne	__start_cpu1
	b	__start_cpu0

__start_cpu1:
#ifdef CPU1_SLEEP
	lis	8, 0x0020	/* SLEEP */
	mfspr	7, SPR_HID0	/* get HID0 */
	or	7, 7, 8
	mtspr	SPR_HID0, 7	/* set HID0 */
	lis	8, 0x0004	/* POW */
	sync
	mtmsr	8
	isync			/* zzz... */
#else
	addi	9, 9, 1
	lis	7, 0x100
__start_cpu1_loop:
	subi	7, 7, 1
	cmpi	0, 7, 0
	bne	__start_cpu1_loop
	lis	8, 0x8000
	ori	8, 8, 0x0c00
	lis	9,_C_LABEL(cpu_info)+CI_CPL@ha
	lwz	9,_C_LABEL(cpu_info)+CI_CPL@l(9)
	stb	9, 0(8)
#endif
	b	__start_cpu1

__start_cpu0:

/* compute end of kernel memory */
#if defined(DDB) || defined(KERNFS) || NKSYMS || defined(MODULAR)
	lis	7,_C_LABEL(startsym)@ha
	addi	7,7,_C_LABEL(startsym)@l
	stw	3,0(7)
	lis	7,_C_LABEL(endsym)@ha
	addi	7,7,_C_LABEL(endsym)@l
	stw	4,0(7)			/* end of symbol table */
#else
	lis	4,_C_LABEL(end)@ha
	addi	4,4,_C_LABEL(end)@l
#endif

	INIT_CPUINFO(4,1,9,0)
	
	lis	3,__start@ha
	addi	3,3,__start@l

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */

	.globl	_C_LABEL(enable_intr)
_C_LABEL(enable_intr):
	mfmsr	3
	ori	3,3,PSL_EE@l
	mtmsr	3
	blr

	.globl	_C_LABEL(disable_intr)
_C_LABEL(disable_intr):
	mfmsr	3
	andi.	3,3,~PSL_EE@l
	mtmsr	3
	blr

	.globl	_C_LABEL(debug_led)
_C_LABEL(debug_led):
	lis	4, 0x8000
	ori	4, 4, 0x0c00
	stb	3, 0(4)
	blr
/*
 * Pull in common switch & setfault code.
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
