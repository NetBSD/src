/*	$NetBSD: rtld_start.S,v 1.2 2000/05/22 19:17:04 matt Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <machine/asm.h>

/* R9 contains the address of PS_STRINGS and since its caller saved,
 * we can just use it.  R6 has a backup copy of the stack pointer which
 * we can use as well.  Since the VAX has perfectly PIC code, we don't
 * need to relocate anything upon startup.
 */
ENTRY(_rtld_start)
	.word	0x0101		/* nops to be safe */

	/* Allocate space on the stack for the cleanup and obj_main
	 * entries that _rtld() will provide for us.
	 */
	subl2	$8, sp
	pushl	sp
	calls	$1, _rtld

	movq	(sp)+, r7	/* grab cleanup and obj_main into r7/r8 */
	jmp	2(r0)		/* jump to entry point + 2 */

/*
 * Lazy binding entry point, called via PLT.
 */
ENTRY(_rtld_bind_start):
	pushr	$0x3f		/* save R0-R5 */
	movq	24(sp),r0	/* get addresses of plt.got & reloc index */
	pushl	(r1)		/* push relocation index */
	pushl	r0		/* push address of obj entry */
	calls	$2,_rtld_bind
	movpsl	-(sp)		/* flush the instruction cache */
	pushab	1f		/*   by issuing an */
	rei			/*   rei. */
1:	movl	r0, 28(sp)	/* save return address onto stack */
	bicw3	6(fp),(r0),r0	/* does the entry mask save any additional regs */
	popr	$0x3f		/* restore r0 to r5 (cond flags aren't modified) */
	bneq	2f		/* yes? do it the hard way */
	addl4	$4,sp		/* no? skip past plt.got on stack */
	addl2	$2,(sp)		/*    skip past the mask */
	rsb			/*    and jump to it */
2:	callg	(ap),*(sp)+	/* return value from _rtld_bind() == actual */
	ret
