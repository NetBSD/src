/*	$NetBSD: ev64260_locore.S,v 1.1 2003/03/05 22:08:26 matt Exp $	*/
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
#include "opt_ppcparam.h"
#include "opt_ipkdb.h"
#include "opt_multiprocessor.h"
#include "opt_openpic.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/oea/hid.h>

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

/*
 * File-scope for locore.S
 */
#ifdef DINK
	.data
dink_return:
	.long	0			/* fake uarea during idle after exit */
dink_stack:
	.long	0			/* fake uarea during idle after exit */
#endif

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
#ifdef DINK
	mflr	0
	lis	8, dink_stack@ha
	stw	1, dink_stack@l(8)
	lis	8, dink_return@ha
	stw	0, dink_return@l(8)
#endif

#if 0
	eieio; sync
	lis	8, 0xf100f104@ha
	lwz	7, -3836(8)
	li	0, 1
	andc	7,7,0
	stw	7, -3836(8)
	eieio; sync
#endif

	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	sync ; isync

	/* Enable data and instruction caches */
	mfspr	8,SPR_HID0
	ori	8, 8, (HID0_ICE | HID0_DCE)@l
	mtspr	SPR_HID0,8

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#if defined(DDB) || defined(KERNFS)
	lis	7,_C_LABEL(startsym)@ha
	addi	7,7,_C_LABEL(startsym)@l
	stw	3,0(7)
	lis	7,_C_LABEL(endsym)@ha
	addi	7,7,_C_LABEL(endsym)@l
	stw	4,0(7)
/*	mr	8,4			* end of symbol table */
#endif
	mr	4,8			/* end of mem reserved for kernel */
	INIT_CPUINFO(4,1,9,0)
	
	lis	3,__start@ha
	addi	3,3,__start@l

	li	5,0
	li	6,0

	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */

#if 0
	.globl	_C_LABEL(extintr_enable)
_C_LABEL(extintr_enable):
	mfmsr	3
	ori	3,3,PSL_EE@l
	sync
	mtmsr	3
	blr

	.globl	_C_LABEL(extintr_restore)
_C_LABEL(extintr_restore):
	mfmsr	4
	or	3,4,3
	sync
	mtmsr	3
	blr

	.globl	_C_LABEL(extintr_disable)
_C_LABEL(extintr_disable):
	mfmsr	3
	andi.	4,3,~PSL_EE@l
	sync
	mtmsr	4
	andi.	3,3,PSL_EE@l
	blr

	.globl	_C_LABEL(mvpppc_reboot)
_C_LABEL(mvpppc_reboot):
	mfmsr	3
	ori	3,3,PSL_IP@l
	sync
	mtmsr	3
	b	0xFFF00100
#endif

#ifdef DINK
	.globl	_C_LABEL(return_to_dink)
_C_LABEL(return_to_dink):
	sync
	lis	4, dink_stack@ha
	lwz	1, dink_stack@l(4)
	lis	4, dink_return@ha
	lwz	5, dink_return@l(4)
	mtlr	5
	blr
#endif

/*
 * Include common switch / setfault code
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Include common trap / execption code
 */
#include <powerpc/powerpc/trap_subr.S>
