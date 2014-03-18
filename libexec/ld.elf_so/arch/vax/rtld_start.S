/*	$NetBSD: rtld_start.S,v 1.17 2014/03/18 23:43:38 matt Exp $	*/

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Portions copyright 2002, 2003 Charles M. Hannum <root@ihack.net>
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
 * we can use as well.
 */
ENTRY(_rtld_start, 0)
	/* Allocate space on the stack for the cleanup and obj_main
	 * entries that _rtld() will provide for us.
	 */
	clrl	%fp
	subl2	$8,%sp

	movab	_DYNAMIC,%r0
	subl3	_GLOBAL_OFFSET_TABLE_,%r0,%r10
	pushl	%r10		/* relocbase */
	pushl	%r0		/* &_DYNAMIC */
	calls	$2,_rtld_relocate_nonplt_self

	pushl	%r10		/* relocbase */
	pushal	4(%sp)		/* sp */
	calls	$2,_rtld	/* entry = _rtld(sp, relocbase) */

	movq	(%sp)+,%r7	/* grab cleanup and obj_main into %r7/%r8 */
	jmp	2(%r0)		/* jump to entry point + 2 */
END(_rtld_start)

/*
 * Lazy binding entry point, called via PLT via JMP into pltgot[1].
 * SP+0: obj entry points
 * SP+4: address to relocation index
 *
 * Note: Some functions rely on there not being an additional call frame;
 * hence the `optimization' to avoid the callg opportunistically.
 */
ALTENTRY(_rtld_bind_start)
	.cfi_startproc
	.cfi_def_cfa 13, 60
	.cfi_offset 16, -56
	.cfi_offset 12, -52
	.cfi_offset 13, -48
	.cfi_offset 15, -44
	.cfi_offset 2, -40
	.cfi_offset 3, -36
	.cfi_offset 4, -32
	.cfi_offset 5, -28
	.cfi_offset 6, -24
	.cfi_offset 7, -20
	.cfi_offset 8, -16
	.cfi_offset 9, -12
	.cfi_offset 10, -8
	.cfi_offset 11, -4
	pushr	$0x3f		/* save R0-R5 */
	movq	24(%sp),%r0	/* get addresses of plt.got & reloc index */
	pushl	(%r1)		/* push relocation index */
	pushl	%r0		/* push address of obj entry */
	calls	$2,_rtld_bind
	movl	%r0,28(%sp)	/* save return address onto stack */
	popr	$0x3f		/* restore R0-R5 (cond flags not modified) */
	addl2	$4,%sp
	callg	(%ap),*(%sp)+	/* return value from _rtld_bind() == actual */
	ret
	.cfi_endproc
END(_rtld_bind_start)
