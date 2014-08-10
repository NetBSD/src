/*      $NetBSD: locore.S,v 1.24.26.1 2014/08/10 06:53:50 tls Exp $   */

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

/*
 * Amiga part written by Adam Ciarcinski.
 * Rewritten for 4.0 and bootargs support by Frank Wille.
 */
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcparam.h"
#include "assym.h"
#include "ksyms.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/hid.h>
#include <amiga/amiga/custom.h>
#include <amigappc/amigappc/p5reg.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define bdneq   bdnzf 2,

/*
 * Globals
 */
	.data
GLOBAL(startsym)
	.long	0			/* start symbol table */
GLOBAL(endsym)
	.long	0			/* end symbol table */
GLOBAL(oeacpufeat)
	.long	0			/* cpu features */
GLOBAL(eclockfreq)
	.long	0			/* needed by the aucc driver only */

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
 *
 * Registers passed from bootloader:
 * r20: E-clock frequency
 * r21: kernel length in bytes (including ksyms, configdevs and memlists)
 * r22: entry (__start)
 * r23: fastmem start
 * r24: fastmem size
 * r25: chipmem size
 * r26: boothowto
 * r27: esym offset
 * r28: machine id << 16
 * r29: amiga flags
 * r30: inhibit sync flags
 * r31: kernel base (__start)
 */
	.globl	__start
__start:
/*
 * Disable FPU/MMU exceptions and set interrupt prefix for a
 * high-vector configuration (exception vectors at 0xfff00000).
 * Also make sure caches are disabled (should be).
 */
	li	0,PSL_IP
	mtmsr	0
	isync

/*
 * Disable M68k
 * First of all we must disable M68k, since it cannot be done by
 * the loader (which is run under M68k).
 */
	lis	5,P5BASE@h
	/* ori	5,5,P5BASE@l	- we assume that the lsb is 0x0000 */
	li	9,P5_M68K_RESET
	stb	9,P5_REG_RESET(5)
	sync

/*
 * Disable Amiga interrupts and DMA
 */
	lis	7,0xbfd000@h
	ori	7,7,0xbfd000@l
	li	8,0x7f
	stb	8,0xd00(7)		/* CIAB icr */
	stb	8,0x1d01(7)		/* CIAA icr */
	sync
	lis	7,0xdff000@h
	ori	7,7,0xdff000@l
	li	8,0x7fff
	sth	8,0x9a(7)		/* INTENA */
	sth	8,0x9c(7)		/* INTREQ */
	sth	8,0x96(7)		/* DMACON */
	sync

/*
 * Change interrupt master
 * Only one CPU can get Amiga interrupts, either M68k or PowerPC.
 */
	li	0,P5_SET_CLEAR|0x7f
	stb	0,P5_INT_LVL(5)
	sync
	li	9,P5_INT_MASTER|P5_ENABLE_IPL
	stb	9,P5_REG_INT(5)		/* PPC is master CPU */
	sync
	li	8,P5_DISABLE_INT
	stb	8,P5_IPL_EMU(5)		/* enable interrupts */
	sync
	li	7,P5_SET_CLEAR|7
	stb	7,P5_IPL_EMU(5)		/* all levels */
	sync

/*
 * Compute start and end of kernel memory
 */
	lis	6,_C_LABEL(end)+3@ha
	addi	6,6,_C_LABEL(end)+3@l
	rlwinm	7,6,0,0,29		/* startsym = end - 32-bit alignment */
#if NKSYMS || defined(DDB) || defined(MODULAR)
	lis	8,_C_LABEL(startsym)@ha
	stw	7,_C_LABEL(startsym)@l(8)
	mr.	27,27			/* esym == 0 ? */
	bne	1f
	sub	27,7,31			/* esym = startsym : no ksyms! */
1:
	add	7,31,27			/* endsym = kernel_base + esym */
	lis	9,_C_LABEL(endsym)@ha
	stw	7,_C_LABEL(endsym)@l(9)	/* end of symbol table */
#endif

	add	4,31,21			/* kernel-end including configdevs */
	INIT_CPUINFO(4,1,9,0)	
	mr	14,4

/*
 * Pass boot arguments
 */
	lis	11,_C_LABEL(eclockfreq)@ha
	stw	20,_C_LABEL(eclockfreq)@l(11)
	mr	3,26			/* boothowto */
	mr	4,23			/* fphystart */
	mr	5,24			/* fphysize */
	mr	6,25			/* cphysize */
					/* esym, already in r7 */
	mr	8,28			/* machine id << 16 */
	mr	9,29			/* amigaflags */
	mr	10,30			/* inhibitsync */
	bl	_C_LABEL(set_boot_args)

	mr	3,31
	mr	4,14
	bl	_C_LABEL(initppc)

/*
 * Enable and invalidate the L1 caches.
 * First the I-caches, then the D-caches. Otherwise the system hangs!!!
 */
	mfspr	8,SPR_HID0
	ori	8,8,HID0_ICE
	ori	9,8,HID0_ICFI
	sync
	mtspr	SPR_HID0,9
	mtspr	SPR_HID0,8
	sync
	isync
	ori	8,8,HID0_DCE
	ori	9,8,HID0_DCFI
	sync
	mtspr	SPR_HID0,9
	mtspr	SPR_HID0,8
	sync

/*
 * Disable serial instr. execution and enable the branch history table for 604
 */
	mfpvr	3
	rlwinm	3,3,16,16,31		/* CPU revision */
	cmpwi	%r3,9			/* 604e? */
	bne	1f
	mfspr	7,SPR_HID0
	ori	7,7,HID0_SGE|HID0_BHT
	ori	6,7,HID0_BTCD
	sync
	mtspr	SPR_HID0,6		/* flush the BTAC */
	sync
	mtspr	SPR_HID0,7
	sync
	isync
1:

/*
 * You are standing at the gate to NetBSD. --More--
 * Unspeakable cruelty and harm lurk down there. --More--
 * Are you sure you want to enter?
 */
	bl	_C_LABEL(main)		/* So be it */

loop:
	b	loop			/* not reached */


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
