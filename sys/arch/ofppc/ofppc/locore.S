/*	$NetBSD: locore.S,v 1.29 2003/02/02 20:43:21 matt Exp $	*/

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
#include "opt_ppcparam.h"
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

/*
 * Globals
 */
GLOBAL(startsym)
	.long	0			/* start of symbol table */
GLOBAL(endsym)
	.long	0			/* end of symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */
GLOBAL(intrnames)
GLOBAL(eintrnames)
GLOBAL(intrcnt)
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
 */
	.text
	.globl	__start
__start:
	bl	_C_LABEL(ofwinit)	/* init our OF hooks */

	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

	/* compute end of kernel memory */
#ifdef DDB
	/* skip symbol table */
	cmpwi	6,0
	beq	1f
	add	9,6,7			/* r9 = args + l */

	/* If the resulting address is misaligned, abort. */
	andi.	8,9,3
	bne	1f			/* misaligned */

	/* First, see if the loader even put the symbols there. */
	lwz	9,-12(9)
	lis	8,0x19730224@h		/* magic number */
	ori	8,8,0x19730224@l
	cmplw	8,9
	bne	1f			/* nope */

	/* Okay, ssym and esym are here -- fetch them. */
	add	9,6,7
	lwz	9,-8(9)
	lis	8,_C_LABEL(startsym)@ha
	stw	9,_C_LABEL(startsym)@l(8)

	add	9,6,7
	lwz	9,-4(9)
	lis	8,_C_LABEL(endsym)@ha
	stw	9,_C_LABEL(endsym)@l(8)

	mr	4,9			/* r8 = end of kernel + symbols */
	b	2f
1:
#endif
	lis	4,_C_LABEL(end)@ha
	addi	4,4,_C_LABEL(end)@l
2:

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

/*
 * Pull in common OFW code.
 */
#include <powerpc/mpc6xx/ofw_subr.S>

/*
 * Pull in common switch / setfault code.
 */
#include <powerpc/powerpc/locore_subr.S>

/*
 * Pull in common trap vector code.
 */
#include <powerpc/powerpc/trap_subr.S>
