/*	$NetBSD: buserr_4060.s,v 1.1 2026/03/14 21:03:40 thorpej Exp $	*/

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
 * bus error and address error handler routines for 68040 and 68060.
 */

#include "opt_m68k_arch.h"

#include <machine/asm.h>
#include <machine/trap.h>

#include "assym.h"

        .file	"buserr_4060.s"
        .text

ENTRY_NOPROFILE(addrerr4060)
	| 68040 pushes a Format 2 frame for address error.
	| 68060 does the same.
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_FMT2_ADDR),%sp@-
	clrl	%sp@-			| dummy code
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

/*
 * bus error handler for 68060
 */
#if defined(M68060)
ENTRY_NOPROFILE(buserr60)
	| 68060 pushes a Format 4 frame for Data or Instruction
	| Access Fault (ATC fault or bus error).
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movel	%sp@(FR_FMT4_FSLW),%d0	| FSLW
	btst	#2,%d0			| branch prediction error?
	jeq	Lnobpe
	movc	%cacr,%d2
	orl	#IC60_CABC,%d2		| clear all branch cache entries
	movc	%d2,%cacr
	movl	%d0,%d1
	andl	#0x7ffd,%d1		| check other faults
	jeq	_ASM_LABEL(faultstkadjnotrap2)
Lnobpe:
| we need to adjust for misaligned addresses
	movl	%sp@(FR_FMT4_FA),%d1	| grab VA
	btst	#27,%d0			| check for mis-aligned access
	jeq	Lberr3			| no, skip
	addl	#28,%d1			| yes, get into next page
					| operand case: 3,
					| instruction case: 4+12+12
					| XXX instr. case not done yet
	andl	#PG_FRAME,%d1		| and truncate
Lberr3:
	movl	%d1,%sp@-
	movl	%d0,%sp@-		| code is FSLW now.
	andw	#0x1f80,%d0
	jeq	_ASM_LABEL(buserr_common) | it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif /* M68060 */

/*
 * bus error handler for 68040
 */
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	| 68040 pushes a Format 7 frame for Access Fault (ATC fault
	| or bus error).
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_FMT7_FA),%d1	| get fault address
	moveq	#0,%d0
	movw	%sp@(FR_FMT7_SSW),%d0	| get SSW
	btst	#11,%d0			| check for mis-aligned
	jeq	Lbe1stpg		| no skip
	addl	#3,%d1			| get into next page
	andl	#PG_FRAME,%d1		| and truncate
Lbe1stpg:
	movl	%d1,%sp@-		| pass fault address.
	movl	%d0,%sp@-		| pass SSW as code
	btst	#10,%d0			| test ATC
	jeq	_ASM_LABEL(buserr_common) | it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif /* M68040 */
