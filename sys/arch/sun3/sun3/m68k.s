/*	$NetBSD: m68k.s,v 1.14 1994/11/21 21:38:45 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 */

/*
 *
 * XXX - Some code at the bottom is from the hp300, should be
 * moved to lib.s  (Which and why, Adam? -gwr)
 *
 * Set processor priority level calls.  Most are implemented with
 * inline asm expansions.  However, we need one instantiation here
 * in case some non-optimized code makes external references.
 * Most places will use the inlined function param.h supplies.
 */

ENTRY(_spl)
	movl	sp@(4),d1
	clrl	d0
	movw	sr,d0
	movw	d1,sr
	rts

ENTRY(getvbr)
	movc vbr, d0
	rts

ENTRY(setvbr)
	movl sp@(4), d0
	movc d0, vbr
	rts

ENTRY(getsr)
	moveq #0, d0
	movw sr, d0
	rts

/*
 * Invalidate instruction cache
 */
ENTRY(ICIA)
	movl	#IC_CLEAR,d0
	movc	d0,cacr			| invalidate i-cache
	rts

ENTRY(DCIU)
	rts

/* void control_copy_byte(caddr_t from, caddr_t to, int size)*/

ENTRY(control_copy_byte)
	movl sp@(4), a0			|a0 = from
	movl sp@(8), a1			|a1 = to 
	movl sp@(12), d1		|d1 = size	
	movl d2, sp@-			| save reg so we can use it for temp
	movc sfc, d0			| save sfc
	movl #FC_CONTROL, d2
	movc d2, sfc
	subqw #1, d1

loop:   movsb a0@+, d2
	movb  d2, a1@+
	dbra d1, loop

	movc d0, sfc
	movl sp@+, d2
	rts

/*	
 * unsigned char get_control_byte (char *)
 */	

ENTRY(get_control_byte)
	movl sp@(4), a0
	movc sfc, d1
	moveq #FC_CONTROL, d0
	movec d0, sfc
	moveq #0, d0
	movsb a0@, d0
	movc d1, sfc
	rts
	
/*
 * unsigned int get_control_word (char *)
 */	

ENTRY(get_control_word)
	movl sp@(4), a0
	movc sfc, d1
	moveq #FC_CONTROL, d0
	movec d0, sfc
	movsl a0@, d0
	movc d1, sfc
	rts

/*	
 * void set_control_byte (char *, unsigned char)
 */

ENTRY(set_control_byte)
	movl sp@(4), a0
	movl sp@(8), d0
	movc dfc, d1
	movl d2, sp@-
	moveq #FC_CONTROL, d2
	movc d2, dfc	
	movsb d0, a0@
	movc d1, dfc
	movl sp@+, d2
	rts

/*
 * void set_control_word (char *, unsigned int)
 */

ENTRY(set_control_word)
	movl sp@(4), a0
	movl sp@(8), d0
	movc dfc, d1
	movl d2, sp@-
	moveq #FC_CONTROL, d2
	movc d2, dfc	
	movsl d0, a0@
	movc d1, dfc
	movc dfc, d1
	movl sp@+, d2
	rts

/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 *
 * [I don't think the ENTRY() macro will do the right thing with this -- glass]
 */
	.globl	_getsp; .align 2
_getsp:
	movl	sp,d0			| get current SP
	addql	#4,d0			| compensate for return address
	rts

	.globl	_getsfc, _getdfc
.align 2
_getsfc:
	movc	sfc,d0
	rts
.align 2
_getdfc:
	movc	dfc,d0
	rts



/*
 * non-local gotos
 */
ENTRY(setjmp)
	movl	sp@(4),a0	| savearea pointer
	moveml	#0xFCFC,a0@	| save d2-d7/a2-a7
	movl	sp@,a0@(48)	| and return address
	moveq	#0,d0		| return 0
	rts

ENTRY(longjmp)
	movl	sp@(4),a0
	moveml	a0@+,#0xFCFC
	movl	a0@,sp@
	moveq	#1,d0
	rts

#ifdef FPCOPROC
/*
 * Save and restore 68881 state.
 * Pretty awful looking since our assembler does not
 * recognize FP mnemonics.
 */
ENTRY(m68881_save)
	movl	sp@(4),a0		| save area pointer
	fsave	a0@			| save state
	tstb	a0@			| null state frame?
	jeq	Lm68881sdone		| yes, all done
	fmovem fp0-fp7,a0@(216)		| save FP general registers
	fmovem fpcr/fpsr/fpi,a0@(312)	| save FP control registers
Lm68881sdone:
	rts

ENTRY(m68881_restore)
	movl	sp@(4),a0		| save area pointer
	tstb	a0@			| null state frame?
	jeq	Lm68881rdone		| yes, easy
	fmovem	a0@(312),fpcr/fpsr/fpi	| restore FP control registers
	fmovem	a0@(216),fp0-fp7	| restore FP general registers
Lm68881rdone:
	frestore a0@			| restore state
	rts
#endif
