/*	$NetBSD: walnut_start.S,v 1.4 2003/02/02 20:43:19 matt Exp $	*/
/*	$OpenBSD: locore.S,v 1.4 1997/01/26 09:06:38 rahnds Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#undef PPC_4XX_NOCACHE

#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "opt_ppcparam.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>
#include <machine/dcr.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/pmap.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

/*
 * Globals
 */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

GLOBAL(intrnames)
	.asciz	"clock", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
	.asciz	"irq16", "irq17", "irq18", "irq19"
	.asciz	"irq20", "irq21", "irq22", "irq23"
	.asciz	"irq24", "irq25", "irq26", "irq27"
	.asciz	"irq28", "softnet", "softclock", "softserial"
	.asciz  "statclock"
GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0
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
	b	1f
	/* Reserve some space for info_block required for IBM eval board bootloader */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

1:
	mr	31,3			/* Save address of PROM info_block */
	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

	/* PPC405GP errata, item #58.
	 * Load string instructions may write incorrect data into the last GPR
	 * targeted in the operation.
	 * Workaround: set OCM0_DSCNTL[DSEN]=0 and OCM0_DSCNTL[DOF]=0 */
	mtdcr	DCR_OCM0_DSCNTL, 0  	/* Disable Data access to OCM */
	mtdcr	DCR_OCM0_ISCNTL, 0  	/* Disable Instruction access to OCM. Just in case */
/*
 * Cpu detect.
 *
 */
__start_cpu0:
#ifdef PPC_4XX_NOCACHE
	/* Disable all caches for physical addresses */
	li	0,0
#else
	/* Allow cacheing for only the first 2GB of RAM */
	lis	0,0xffff
#endif
	mtdccr	0
	mticcr	0

	/* Invalidate all TLB entries */
	tlbia
	sync
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#if defined(DDB)
	/* If we had symbol table location we'd store it here and would've adjusted r8 here */
	lis	7,_C_LABEL(startsym)@ha
	addi	7,7,_C_LABEL(startsym)@l
	stw	8,0(7)
	lis	7,_C_LABEL(endsym)@ha
	addi	7,7,_C_LABEL(endsym)@l
	stw	8,0(7)
#endif

	/*
	 * Set up TLB entry to cover kernel addresses.
	 *
	 * XXX: Skip TLB 0 for now, due to unresolved TLB 0 replacement
	 *	and hard hangs
	 */
	li	0,1
	mtpid	0
	sync

	li	0,0
#ifdef PPC_4XX_NOCACHE
	li	4,TLB_EX|TLB_WR|TLB_I /* |TLB_W */
#else
	li	4,TLB_EX|TLB_WR /* |TLB_W */
#endif
        li	3,TLB_VALID|TLB_PG_16M
        tlbwe	4,0,1			/* Load the data(Low) portion of the entry */
        tlbwe	3,0,0			/* Load the tag(High) portion of the entry */

#if 1
	/* Damn. Have to be able to access all real memory.... Hardcode for 32M for now. */
	li	0,1
	lis	4,0x01000000@h
	ori	3,4,0
#ifdef PPC_4XX_NOCACHE
	addi	4,4,TLB_EX|TLB_WR|TLB_I /* |TLB_W */
#else
	addi	4,4,TLB_EX|TLB_WR /* |TLB_W */
#endif
	addi	3,3,TLB_VALID|TLB_PG_16M
	tlbwe	4,0,1			/* Load the data(Low) portion of the entry */
	tlbwe	3,0,0			/* Load the tag(High) portion of the entry */
#endif

	/* set up a TLB mapping to cover uart0 */
	lis	3,0xef000000@h		/* Load the virtual address */
	ori	4,3,0			/* Load the physical address */

	clrrwi	4,4,10			/* Mask off the real page number */
	/* write, execute, cache inhibit, guarded */
	ori	4,4,(TLB_WR|TLB_EX|TLB_I|TLB_G)

	clrrwi	3,3,10			/* Mask off the effective page number */
	ori	3,3,(TLB_VALID|TLB_PG_16M)

	li	0,2

	tlbwe	4,0,1			/* Load the data portion of the entry */
	tlbwe	3,0,0			/* Load the tag portion of the entry */

	/* END of TLB setup */

	INIT_CPUINFO(8,1,9,0)
	mr	4,8

	lis	3,__start@ha
	addi	3,3,__start@l

	mr	6,31			/* info_block address */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */

#include <powerpc/ibm4xx/4xx_locore.S>
