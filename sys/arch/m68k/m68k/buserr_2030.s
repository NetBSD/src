/*	$NetBSD: buserr_2030.s,v 1.1 2026/03/14 21:03:40 thorpej Exp $	*/

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
 * bus error and address error handler routines for 68020 (with various
 * MMU types) and 68030.
 */

#include "opt_m68k_arch.h"

#include <machine/asm.h>
#include <machine/trap.h>

#include "assym.h"

        .file	"buserr_2030.s"
        .text

#if !defined(M68K_MMU_MOTOROLA) && !defined(M68K_MMU_HP) &&	\
    !defined(M68K_MMU_SUN3)
#error Why are we even here?
#endif

#define	MULTIMMU	((defined(M68K_MMU_MOTOROLA) +		\
			  defined(M68K_MMU_HP) +		\
			  defined(M68K_MMU_SUN3))		\
			 > 1)

ENTRY_NOPROFILE(busaddrerr2030)
GLOBAL(buserr2030)
GLOBAL(addrerr2030)
	| 68020 and 68030 may push one of two different stack frames
	| for bus error and address error:
	|
	| Format A - execution unit is at an instruction boundary
	|
	| Format B - instruction execution is in-progress
	|
	| Happily, the stuff we need is at the same offset in both
	| stack frame types.
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	moveq	#0,%d0
	movw	%sp@(FR_FMTA_SSW),%d0	| grab SSW for fault processing
	btst	#12,%d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,%d0			| yes, must set FB
	movw	%d0,%sp@(FR_FMTA_SSW)	| for hardware too
LbeX0:
	btst	#13,%d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,%d0			| yes, must set FC
	movw	%d0,%sp@(FR_FMTA_SSW)	| for hardware too
LbeX1:
	btst	#8,%d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	%sp@(FR_FMTA_DCFA),%d1	| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,%sp@(FR_FORVEC)	| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	%sp@(FR_PC),%d1		| no, can use save PC
	btst	#14,%d0			| FB set?
	jeq	Lbe3			| no, try FC
	addql	#4,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe3:
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, done
	addql	#2,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe4:
	movl	%sp@(FR_FMTB_SBA),%d1	| long format, use stage B address
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,%d1			| yes, adjust address
Lbe10:
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%sp@(8+FR_FORVEC),%d0	| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it

#if defined(M68K_MMU_SUN3)
/*
 * Sun3 MMU handling.
 */
#if MULTIMMU
#error No MULTIMMU configuration for Sun3.
#endif
Lbuserr2030_sun3_mmu:
	clrl	%d0			| make sure top bits are cleared too
	movl	%d1,%sp@-		| save %d1
	movc	%sfc,%d1		| save %sfc to %d1
	moveq	#FC_CONTROL,%d0		| %sfc = FC_CONTROL
	movc	%d0,%sfc
	movsb	BUSERR_REG,%d0		| get value of bus error register
	movc	%d1,%sfc		| restore %sfc
	movl	%sp@+,%d1		| restore %d1
	andb	#BUSERR_MMU,%d0		| is this an MMU fault?
	jeq	_ASM_LABEL(buserr_common) | non-MMU bus error
#if MULTIMMU
	jra	Lismerr			| MMU fault
#endif
#endif /* M68K_MMU_SUN3 */

#if defined(M68K_MMU_HP)
/*
 * HP MMU handling.
 */
#if MULTIMMU
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jne	Lbuserr2030_moto_mmu
#endif
Lbuserr2030_hp_mmu:
	movl	_C_LABEL(MMUbase),%a0	| %a0 = MMU address
	movl    %a0@(MMUSTAT),%d0       | read MMU status
        btst	#3,%d0			| MMU fault?
	jeq	_ASM_LABEL(buserr_common) | no, just a non-MMU bus error
	andl	#~MMU_FAULT,%a0@(MMUSTAT)| yes, clear fault bits
	movw	%d0,%sp@		| pass MMU stat in upper half of code
#if MULTIMMU
	jra	Lismerr			| and handle it
#endif
#endif /* M68K_MMU_HP */

#if defined(M68K_MMU_MOTOROLA)
/*
 * 68851 / 68030 MMU handling.
 * A.K.A. the default case.
 */
Lbuserr2030_moto_mmu:
	movl	%d1,%a0			| fault address
	movl	%sp@,%d0		| function code from ssw
	btst	#8,%d0			| data fault?
	jne	Lbe10a
	movql	#1,%d0			| user program access FC
					| (we dont separate data/program)
	btst	#5,%sp@(8+FR_SR)	| supervisor mode?
	jeq	Lbe10a			| if no, done
	movql	#5,%d0			| else supervisor program access
Lbe10a:
#ifdef M68030
	ptestr	%d0,%a0@,#0		| only PTEST #0 can detect transparent
	pmove	%psr,%sp@		|   translation (TT0 or TT1).
	movw	%sp@,%d1
	btst	#6,%d1			| transparent (TT0 or TT1)?
	jne	Lisberr			| yes -> bus error
#endif /* M68030 */
	ptestr	%d0,%a0@,#7		| no, do a table search
	pmove	%psr,%sp@		| save result
	movb	%sp@,%d1
	btst	#2,%d1			| invalid (incl. limit viol. and berr)?
	jeq	Lmightnotbemerr		| no -> wp check
	btst	#7,%d1			| is it MMU table berr?
	jeq	Lismerr			| no, must be fast
	jra	Lisberr			| real bus err needs not be fast.
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr			| no, must be bus error
	movl	%sp@,%d0		| ssw into low word of d0
	andw	#0xc0,%d0		| write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jeq	Lisberr			| yes, was not WPE, must be bus err
#endif /* M68K_MMU_MOTOROLA */

Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#ifdef M68K_MMU_MOTOROLA
Lisberr:
	clrw	%sp@			| re-clear upper half of padded
					| SSW word that we clobbered
					| with PMOVE.
	jra	_ASM_LABEL(buserr_common)
#endif
