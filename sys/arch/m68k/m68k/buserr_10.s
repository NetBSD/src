/*	$NetBSD: buserr_10.s,v 1.1 2026/03/14 21:03:40 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

/*
 * bus error and address error handler routines for 68010 (with various
 * MMU types).
 */

#include "opt_m68k_arch.h"

#include <machine/asm.h>
#include <machine/trap.h>

#include "assym.h"

        .file	"buserr_10.s"
        .text

#if !defined(M68K_MMU_SUN2)
#error Why are we even here?
#endif

#define	MULTIMMU	((defined(M68K_MMU_SUN2))		\
			 > 1)

ENTRY_NOPROFILE(busaddrerr10)
GLOBAL(buserr10)
GLOBAL(addrerr10)
	| 68010 always pushes a Format 8 frame for these exceptions.
	clrl	%sp@-			| stack adjust count + pad word
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	moveq	#0,%d0
	movw	%sp@(FR_FMT8_SSW),%d0	| grab SSW for fault processing
	movl	%sp@(FR_FMT8_ACCADDR),%d1 | fault address is as given in frame
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%sp@(FR_FORVEC),%d0	| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it

#if defined(M68K_MMU_SUN2)
/*
 * Sun2 MMU handling.
 */
#if MULTIMMU
#error No MULTIMMU configuration for Sun2.
#endif
Lbuserr10_sun2_mmu:
	clrl	%d0			| make sure top bits are cleared too
	movl	%d1,%sp@-		| save %d1
	movc	%sfc,%d1		| save %sfc to %d1
	moveq	#FC_CONTROL,%d0		| %sfc = FC_CONTROL
	movc	%d0,%sfc
	movsw	BUSERR_REG,%d0		| get value of bus error register
	movc	%d1,%sfc		| restore %sfc
	movl	%sp@+,%d1		| restore %d1
#ifdef DEBUG
	movw	%d0,_C_LABEL(buserr_reg) | save bus error register value
#endif
	andb	#BUSERR_PROTERR,%d0	| is this an MMU
					|  (protection *or* page unavailable)
					|  fault?
	jeq	_ASM_LABEL(buserr_common) | non-MMU bus error
#if MULTIMMU
	jra	Lismerr			| MMU fault
#endif
#endif /* M68K_MMU_SUN2 */

Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
