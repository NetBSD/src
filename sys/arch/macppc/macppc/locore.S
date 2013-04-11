/*	$NetBSD: locore.S,v 1.71 2013/04/11 18:36:44 macallan Exp $	*/

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
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_altivec.h"
#include "opt_ppcparam.h"
#include "ksyms.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>

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
	bl	_C_LABEL(cpu_model_init)	/* init oeacpufeat */
	bl	_C_LABEL(ofwinit)		/* init OF */

	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
#if defined(PMAP_OEA64_BRIDGE)
	mfmsr	0			/* Clear SF and ISF bits */
	clrldi	0,0,3
	mtmsrd	0
#endif /* PMAP_OEA64_BRIDGE */
	isync

/* compute end of kernel memory */
	lis	4,_C_LABEL(end)@ha
	addi	4,4,_C_LABEL(end)@l
#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* skip symbol table */
	cmpwi	6,0
	beq	1f
	add	9,6,7			/* r9 = args + l */
	lwz	9,-8(9)			/* esym */
	cmpwi	9,0
	beq	1f
	mr	4,9
1:
#endif

#if defined (PMAC_G5) || defined (MAMBO)
	 /* and clear HID5 DCBZ bits (56/57), need to do this early */
	mfspr	11,SPR_HID5
	rldimi	11,0,6,56
	sync
	mtspr	SPR_HID5,11
	isync
	sync

	/* Setup HID1 features, prefetch + i-cacheability controlled by PTE */
	mfspr	0,SPR_HID1
	li	11,0x1200
	sldi	11,11,44
	or	0,0,11
	mtspr	SPR_HID1,0
	isync
	sync

	/* Restore r0 */
	li	0,0
#endif /* PMAC_G5 */


	/*
	 * Initialize cpu_info[0]
	 */
	INIT_CPUINFO(4,1,9,0)

	lis	3,__start@ha
	addi	3,3,__start@l
	mr	5,6			/* args string */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)
	b	_C_LABEL(OF_exit)

#if defined (PPC_OEA64_BRIDGE)
/* Return all 64 bits of HID4 */
	.text
	.globl _C_LABEL(mfhid4)
_C_LABEL(mfhid4):
	mfspr	4, SPR_HID4 
	std	4, 0(3)
	blr

/* Set all 64 bits of HID4 */
	.text
	.globl _C_LABEL(mthid4)
_C_LABEL(mthid4):
	ld 4, 0(3)
	sync
	mtspr SPR_HID4, 4 
	isync 
	isync
	blr

	.text
	.globl _C_LABEL(change_hid4)
_C_LABEL(change_hid4):
	mfspr 6, SPR_HID4
	rldicl 5, 6, 32, 0 
	ori 5, 5, 0x100
	rldicl 5, 5, 32, 0 
	std 5, 0(3)
	blr
#endif

/*
 * Pull in common Open Firmware code.
 */
#include <powerpc/oea/ofw_subr.S>

/*
 * Pull in common switch and setfault code.
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
