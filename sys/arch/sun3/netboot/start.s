/*
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
 *
 * $Header: /cvsroot/src/sys/arch/sun3/netboot/Attic/start.s,v 1.1 1993/10/12 05:24:01 glass Exp $
 */

#include "assym.s"
#include "../include/asm.h"

.text
.globl start
start:
	movl #FIXED_LOAD_ADDR,a0	| where we are (a0)
	lea start:l, a1			| where we want to be (a1)	
	cmpl a0, a1
	jeq begin
	movl #_edata, d0
copy:
	movl a0@+,a1@+
	cmpl d0, a1
	jne copy
	jmp begin:l
/* find out where we are, and copy ourselves to where we are supposed to be */
.align 4
begin:	
	moveq #FC_CONTROL, d0		| make movs get us to the control
	movc d0, dfc			| space where the sun3 designers
	movc d0, sfc			| put all the "useful" stuff
	moveq #CONTEXT_0, d0
	movsb d0, CONTEXT_REG		| now in context 0
	
savesp: movl sp, start-4:l
	lea start-4:l, sp
	movl #(FIXED_LOAD_ADDR-4), sp
	jsr _machdep_nfsboot
	jsr FIXED_LOAD_ADDR

/*
 * unsigned int get_control_word (char *)
 */	

ENTRY(get_control_word)
	movl sp@(4), a0
	movsl a0@, d0
	rts

/*
 * void set_control_word (char *, unsigned int)
 */

ENTRY(set_control_word)
	movl sp@(4), a0
	movl sp@(8), d0
	movsl d0, a0@
	rts

/*	
 * unsigned char get_control_byte (char *)
 */	

ENTRY(get_control_byte)
	movl sp@(4), a0
	moveq #0, d0
	movsb a0@, d0
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
