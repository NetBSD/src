/*	$NetBSD: _context_u.S,v 1.2 2003/01/19 21:31:42 matt Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "assym.h"

ENTRY(_getcontext_u, 0)
	movl	4(%ap),%r0
	movl	16(%sp),%r3				/* save return pc */
	movl	%r3,%r2					/* where to return */
getc:
	movab	1f,16(%fp)
	ret
1:
	_ALIGN_TEXT
	clrl	(UC_GREGS + _REG_R0)(%r0)		/* r0 */
	movq	%r6,(UC_GREGS + _REG_R6)(%r0)		/* r6/r7 */
	movq	%r8,(UC_GREGS + _REG_R8)(%r0)		/* r8/r9 */
	movq	%r10,(UC_GREGS + _REG_R10)(%r0)		/* r10/r11 */
	movq	%ap,(UC_GREGS + _REG_AP)(%r0)		/* ap/sp */
	movl	%fp,(UC_GREGS + _REG_FP)(%r0)		/* fp */
	movl	%r2,(UC_GREGS + _REG_PC)(%r0)		/* pc */
	movpsl	(UC_GREGS + _REG_PSL)(%r0)		/* psl(high) */
	movw	6(%fp),(UC_GREGS + _REG_PSL)(%r0)	/* psl(low) */

	bisl2	$(_UC_CPU+_UC_USER),(UC_FLAGS)(%r0)
	clrl	%r0
	jmp	(%r3)

ENTRY(_setcontext_u, 0)
	movl	4(%ap),%r1
setc:
	bitl	$(_UC_USER),(UC_FLAGS)(%r1)
	bneq	1f
	pushl	%r1
	calls	$1,_C_LABEL(setcontext)
	halt

1:	movl	(UC_GREGS + _REG_R0)(%r1),%r0		/* r0 */
	movq	(UC_GREGS + _REG_R6)(%r1),%r6		/* r6/r7 */
	movq	(UC_GREGS + _REG_R8)(%r1),%r8		/* r8/r9 */
	movq	(UC_GREGS + _REG_R10)(%r1),%r10		/* r10/r11 */
	movq	(UC_GREGS + _REG_AP)(%r1),%ap		/* ap/sp */
	movl	(UC_GREGS + _REG_FP)(%r1),%fp		/* fp */
	movq	(UC_GREGS + _REG_PC)(%r1),-(%sp)	/* pc/psl */
	rei						/* return */

ENTRY(_swapcontext_u, 0)
	movq	4(%ap),%r0
	movl	16(%sp),%r2				/* save return pc */
	movab	setc,%r3				/* where to return */
	brw	getc
