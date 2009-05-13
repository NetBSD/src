/*	$NetBSD: locore.S,v 1.8.4.1 2009/05/13 17:18:08 jym Exp $	*/
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

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#define _NOREGNAMES
#include <machine/asm.h>

#include <powerpc/spr.h>

#include "ksyms.h"

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
	.long	0			/* cpu features */

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
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Startup entry.  Note, this must be the first thing in the text
 * segment!
 *
 * The bootloaded passes the following registers:
 *
 *  %r3 == start of symbol table
 *  %r4 == end of symbol table
 *  %r5 == boot parameters
 */
	.text
	.globl	__start
__start:
	li	%r0,0
	mtmsr	%r0			/* Disable FPU/MMU/exceptions */
	isync

/* compute end of kernel memory */
#if NKSYMS || defined(DDB) || defined(MODULAR) || defined(KERNFS)
	lis	%r7,_C_LABEL(startsym)@ha
	addi	%r7,%r7,_C_LABEL(startsym)@l
	stw	%r3,0(%r7)
	lis	%r7,_C_LABEL(endsym)@ha
	addi	%r7,%r7,_C_LABEL(endsym)@l
	stw	%r4,0(%r7)		/* end of symbol table */
#else
	lis	%r4,_C_LABEL(end)@ha
	addi	%r4,%r4,_C_LABEL(end)@l
#endif

	INIT_CPUINFO(%r4,%r1,%r9,%r0)

	lis	%r3,__start@ha
	addi	%r3,%r3,__start@l

	bl	_C_LABEL(initppc)

/* enable internal i/d-cache */
	mfpvr	%r9
	rlwinm	%r9,%r9,16,16,31
	cmpi	%cr0,%r9,1
	beq	3f			/* not needed for 601 */
	mfspr	%r11,SPR_HID0
	andi.	%r0,%r11,HID0_DCE
	ori	%r11,%r11,HID0_ICE|HID0_DCE
	ori	%r8,%r11,HID0_ICFI
	bne	1f			/* don't invalidate the D-cache */
	ori	%r8,%r8,HID0_DCFI		/* unless it wasn't enabled */
1:
	sync
	mtspr	SPR_HID0,%r8		/* enable and invalidate caches */
	sync
	mtspr	SPR_HID0,%r11		/* enable caches */
	sync
	isync
	cmpi	%cr0,%r9,4		/* check for 604 */
	cmpi	%cr1,%r9,9		/* or 604e */
	cmpi	%cr2,%r9,10		/* or mach5 */
	cror	2,2,6
	cror	2,2,10
	bne	3f
	ori	%r11,%r11,HID0_SIED|HID0_BHTE /* for 604[e], enable */
	bne	%cr2,2f
	ori	%r11,%r11,HID0_BTCD
2:
	mtspr	SPR_HID0,%r11
3:
	sync
	isync

	bl	_C_LABEL(main)
	b	bugret

/*
 * void _mvmeppc_unsup_board(const char *msg, const char *msgend)
 *
 * Print the specified message using ppcbug's console output syscall
 * before dropping back out to bug.
 */
ENTRY(_mvmeppc_unsup_board)
	addi	%r10,0,0x0022
	sc
bugret:	addi	%r10,0,0x0063
	sc
	/* NOTREACHED */

	.globl	_C_LABEL(enable_intr)
_C_LABEL(enable_intr):
	mfmsr	%r3
	ori	%r3,%r3,PSL_EE@l
	mtmsr	%r3
	blr

	.globl	_C_LABEL(disable_intr)
_C_LABEL(disable_intr):
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l
	mtmsr	%r3
	blr

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
