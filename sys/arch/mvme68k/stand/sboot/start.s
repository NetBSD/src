/*
 * $NetBSD: start.s,v 1.5.16.1 2000/12/08 09:28:44 bouyer Exp $
 *
 * Copyright (c) 1995 Charles D. Cranor
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
 *      This product includes software developed by Charles D. Cranor.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * start: at address 0x4000, load at 0xa000 (so put stack at 0x9ff0)
 */

#include <m68k/asm.h>

	.text
ASENTRY_NOPROFILE(_start)
ASENTRY_NOPROFILE(start)
	movb	#0,_C_LABEL(reboot)
	jra	Ldoit
restart:
	movb	#1,_C_LABEL(reboot) | fall through
	
Ldoit:
	movl	#0x00006ff0,%sp
	jsr _C_LABEL(sboot)

Lname:
	.ascii "sboot\0"

ENTRY_NOPROFILE(go)
	clrl	%d0		| dev lun
	clrl	%d1		| ctrl lun
	movl	#0x2c, %d4	| flags for IPL
	movl	#0xfffe1800, %a0	| address of "disk" ctrl
	movl	%sp@(4), %a1	| entry point of loaded program
	movl	%d0, %a2	| media config block (NULL)
	movl	%sp@(8), %a3	| nb args (start)
	movl	%sp@(12), %a4	| nb end args
	movl	#Lname, %a5	| args
	movl	#Lname+5, %a6	| end args
				| SRT0 will set stack
	jmp	%a1@		| GO!
