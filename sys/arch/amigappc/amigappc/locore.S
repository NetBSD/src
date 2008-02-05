/*      $NetBSD: locore.S,v 1.15 2008/02/05 22:31:48 garbled Exp $   */

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
 *      This product includes software developed by TooLs GmbH.
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

/* Amiga part written by Adam Ciarcinski adam@albedo.art.pl */

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define bdneq   bdnzf 2,

#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#define _NOREGNAMES
#include <machine/asm.h>
#include <machine/p5reg.h>

#include <machine/hid.h>
#include <amiga/amiga/custom.h>

#include <powerpc/spr.h>

	.data
GLOBAL(proc0paddr)
	.long	0
GLOBAL(oeacpufeat)
	.long	0				/* cpu features */
GLOBAL(ssir)
	.word	0
GLOBAL(delaydivisor)
	.long	0
GLOBAL(lowram)
	.long	0

GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0
GLOBAL(eintrcnt)


idle_u:
	.long	0			/* fake uarea during idle after exit */
openfirmware_entry:
	.long	0			/* openfirmware entry point */
srsave:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
        .text
        .globl  _C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Startup entry
 * This is where bootloader jumps after successful kernel loading.
 * Must be the first address in text segment
 */
	.text
	.globl	__start
__start:
/*
 * Disable M68k
 * First of all we must disable M68k, since it cannot be done by
 * the loader (which is run under M68k).
 */
	lis	5,P5BASE@h
	/* ori	5,5,P5BASE@l	- we assume that the lsb is 0x0000 */
	li	3,P5_M68K_RESET
	stb	3,P5_REG_RESET(5)

/*
 * Disable Amiga interrupts
 */
	lis	4,0xbfd000@h
	ori	4,4,0xbfd000@l
	li	3,0x7f
	stb	3,0xd00(4)	/* CIAB icr */
	stb	3,0x1d01(4)	/* CIAA icr */

/*
 * Change interrupt master
 * Only one CPU can get Amiga interrupts, either M68k or PowerPC.
 */
	li	3,P5_SET_CLEAR|P5_DISABLE_INT
	stb	3,P5_IPL_EMU(5)	/* disable interrupts */
	li	3,P5_INT_MASTER
	stb	3,P5_REG_INT(5)	/* let the PowerPC get interrupts */
	li	3,P5_DISABLE_INT
	stb     3,P5_IPL_EMU(5)	/* enable interrupts */
	li	3,P5_LVL7
	stb	3,P5_INT_LVL(5)

/*
 * Remove ROM mapping
 * This will give us extra 512K RAM
 */
	li	3,0x60
	stb	3,P5_REG_LOCK(5)
	li	3,0x50
	stb	3,P5_REG_LOCK(5)
	li	3,0x30
	stb	3,P5_REG_LOCK(5)
	li	3,P5_SET_CLEAR|P5_SHADOW
	stb	3,P5_REG_SHADOW(5)
	li	3,0x00
	stb	3,P5_REG_LOCK(5)


/*
 * Clear BAT registers
 * Clearing segment register
 */
	xor	0,0,0
	mtibatu	0,0
	mtibatu	1,0
	mtibatu	2,0
	mtibatu	3,0
	mtdbatu	0,0
	mtdbatu	1,0
	mtdbatu	2,0
	mtdbatu	3,0
	mtsr	0,0
	mtsr	1,0
	mtsr	2,0
	mtsr	3,0
	mtsr	4,0
	mtsr	5,0
	mtsr	6,0
	mtsr	7,0
	mtsr	8,0
	mtsr	9,0
	mtsr	10,0
	mtsr	11,0
	mtsr	12,0
	mtsr	13,0
	mtsr	14,0
	mtsr	15,0
	sync

/*
 * Enable caches on 603e/604e
 */
	mfspr	3,1008
	ori	3,3,HID0_ICE|HID0_DCE|HID0_SGE|HID0_BHT
	ori	4,3,HID0_ICFI|HID0_DCFI
	sync
	mtspr	1008,4
	sync
	mtspr	1008,3
	sync
	isync

/*
 * Compute end-of-kernel memory
 */
	lis	8,_C_LABEL(end)@h
	ori	8,8,_C_LABEL(end)@l
	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9		/* page alignment */

	addi	8,8,USPACE	/* space for idle_u */
	lis	9,_C_LABEL(proc0paddr)@ha
	stw	8,_C_LABEL(proc0paddr)@l(9)
	addi	1,8,USPACE-FRAMELEN	/* stack pointer for proc0 */
	mr	4,1
	xor	0,0,0
	stwu	0,-16(1)

	lis	3,__start@h
	ori	3,3,__start@l
	bl	_C_LABEL(initppc)

/*
 * Jump to main() (from kern/init_main.c)
 * main() should call consinit(), uvm_init() and cpu_startup()
 */
	b	_C_LABEL(main)

loop:
	b	loop			/* not reached */

/*
 * Colours on the screen
 * For visual test purposes.
 */
	.globl	_C_LABEL(cpu_fail)
_C_LABEL(cpu_fail):
	li	3,0xb8b
	li	4,0xa0a
	li	5,0x000
	lis	2,0xdff000@h
	ori	2,2,0xdff000@l
loop1:	sth	3,0x180(2)
	sth	4,0x180(2)
	sth	5,0x180(2)
	b	loop1


/*
 * ADAM: fake interrupt handler
 */
	.globl	_C_LABEL(adamint),_C_LABEL(adamintsize)
_C_LABEL(adamint):
	mtsprg	2,2
	mtsprg	3,3
	lis	3,0xdff000@h
	ori	3,3,0xdff000@l
1:	li	2,0x00a
	sth	2,0x180(3)
	li	2,0xaaa
	sth	2,0x180(3)
	b	1b
	mfsprg	3,3
	mfsprg	2,2
	rfi
_C_LABEL(adamintsize) = .-_C_LABEL(adamint)


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
