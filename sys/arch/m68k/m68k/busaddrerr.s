/*	$NetBSD: busaddrerr.s,v 1.1.10.2 2014/08/20 00:03:10 tls Exp $	*/

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
 * bus error and address error handler routines common to all m68k ports.
 */

/*
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.s, like so:
 *
 *	#include <m68k/m68k/busaddrerr.s>
 */

/* assume M68K_MMU_MOTOROLA is default if none is defined */
#if !defined(M68K_MMU_MOTOROLA) && !defined(M68K_MMU_HP)
#define M68K_MMU_MOTOROLA
#endif

/*
 * address error handler for 68040/68060
 */
#if defined(M68040) || defined(M68060)
ENTRY_NOPROFILE(addrerr4060)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+8),%sp@-
	clrl	%sp@-			| dummy code
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#endif

/*
 * bus error handler for 68060
 */
#if defined(M68060)
ENTRY_NOPROFILE(buserr60)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movel	%sp@(FR_HW+12),%d0	| FSLW
	btst	#2,%d0			| branch prediction error?
	jeq	Lnobpe
	movc	%cacr,%d2
	orl	#IC60_CABC,%d2		| clear all branch cache entries
	movc	%d2,%cacr
	movl	%d0,%d1
#if defined(amiga) || defined(atari)
	addql	#1,L60bpe
#endif
	andl	#0x7ffd,%d1		| check other faults
	jeq	_ASM_LABEL(faultstkadjnotrap2)
Lnobpe:
| we need to adjust for misaligned addresses
	movl	%sp@(FR_HW+8),%d1	| grab VA
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
	jeq	Lberr60			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr60:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
#ifdef mac68k
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+8),_C_LABEL(m68k_fault_addr) | save fault addr
#endif
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif

/*
 * bus error handler for 68040
 */
#if defined(M68040)
ENTRY_NOPROFILE(buserr40)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	movl	%sp@(FR_HW+20),%d1	| get fault address
	moveq	#0,%d0
	movw	%sp@(FR_HW+12),%d0	| get SSW
	btst	#11,%d0			| check for mis-aligned
	jeq	Lbe1stpg		| no skip
	addl	#3,%d1			| get into next page
	andl	#PG_FRAME,%d1		| and truncate
Lbe1stpg:
	movl	%d1,%sp@-		| pass fault address.
	movl	%d0,%sp@-		| pass SSW as code
	btst	#10,%d0			| test ATC
	jeq	Lberr40			| it is a bus error
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lberr40:
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
#ifdef mac68k
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+20),_C_LABEL(m68k_fault_addr) | save fault addr
#endif
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif

/*
 * bus error and address error handlers for 68020/68030
 */
#if defined(M68020) || defined(M68030)
ENTRY_NOPROFILE(busaddrerr2030)
	clrl	%sp@-			| stack adjust count
	moveml	%d0-%d7/%a0-%a7,%sp@-	| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(FR_SP)		|   in the savearea
	moveq	#0,%d0
	movw	%sp@(FR_HW+10),%d0	| grab SSW for fault processing
	btst	#12,%d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,%d0			| yes, must set FB
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX0:
	btst	#13,%d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,%d0			| yes, must set FC
	movw	%d0,%sp@(FR_HW+10)	| for hardware too
LbeX1:
	btst	#8,%d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	%sp@(FR_HW+16),%d1	| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,%sp@(FR_HW+6)	| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	%sp@(FR_HW+2),%d1	| no, can use save PC
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
	movl	%sp@(FR_HW+36),%d1	| long format, use stage B address
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,%d1			| yes, adjust address
Lbe10:
	movl	%d1,%sp@-		| push fault VA
	movl	%d0,%sp@-		| and padded SSW
	movw	%sp@(FR_HW+8+6),%d0	| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it
#if defined(M68K_MMU_MOTOROLA)
#if defined(M68K_MMU_HP)
	tstl	_C_LABEL(mmutype)	| HP MMU?
	jeq	Lbehpmmu		| yes, different MMU fault handler
#endif
	movl	%d1,%a0			| fault address
	movl	%sp@,%d0		| function code from ssw
	btst	#8,%d0			| data fault?
	jne	Lbe10a
	movql	#1,%d0			| user program access FC
					| (we dont separate data/program)
	btst	#5,%sp@(FR_HW+8)	| supervisor mode?
	jeq	Lbe10a			| if no, done
	movql	#5,%d0			| else supervisor program access
Lbe10a:
	ptestr	%d0,%a0@,#7		| do a table search
	pmove	%psr,%sp@		| save result
	movb	%sp@,%d1
	btst	#2,%d1			| invalid (incl. limit viol. and berr)?
	jeq	Lmightnotbemerr		| no -> wp check
	btst	#7,%d1			| is it MMU table berr?
	jne	Lisberr1		| yes, needs not be fast.
#endif /* M68K_MMU_MOTOROLA */
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
#if defined(M68K_MMU_MOTOROLA)
Lmightnotbemerr:
	btst	#3,%d1			| write protect bit set?
	jeq	Lisberr1		| no, must be bus error
	movl	%sp@,%d0		| ssw into low word of d0
	andw	#0xc0,%d0		| write protect is set on page:
	cmpw	#0x40,%d0		| was it read cycle?
	jne	Lismerr			| no, was not WPE, must be MMU fault
	jra	Lisberr1		| real bus err needs not be fast.
#endif /* M68K_MMU_MOTOROLA */
#if defined(M68K_MMU_HP)
Lbehpmmu:
	MMUADDR(%a0)
	movl    %a0@(MMUSTAT),%d0       | read MMU status
        btst	#3,%d0			| MMU fault?
	jeq	Lisberr1		| no, just a non-MMU bus error
	andl	#~MMU_FAULT,%a0@(MMUSTAT)| yes, clear fault bits
	movw	%d0,%sp@		| pass MMU stat in upper half of code
	jra	Lismerr			| and handle it
#endif
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it
Lisberr1:
	clrw	%sp@			| re-clear pad word
	tstl	_C_LABEL(nofault)	| catch bus error?
	jeq	Lisberr			| no, handle as usual
#ifdef mac68k
	movl	%a2,_C_LABEL(mac68k_a2_fromfault) | save %a2
	movl	%sp@(FR_HW+8+16),_C_LABEL(m68k_fault_addr) | save fault addr
#endif
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
#endif /* M68020 || M68030 */

Lisberr:				| also used by M68040/60
	movl	#T_BUSERR,%sp@-		| mark bus error
	jra	_ASM_LABEL(faultstkadj)	| and deal with it

