/*	$NetBSD: brk.S,v 1.7 2001/05/11 19:58:08 msaitoh Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)brk.s	5.2 (Berkeley) 12/17/90
 */

#include <machine/asm.h>
#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: brk.S,v 1.7 2001/05/11 19:58:08 msaitoh Exp $")
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(brk,_brk)
#endif

	.data
	.globl	_C_LABEL(__minbrk)
	.align	2
_C_LABEL(__minbrk):
	.long	_end

	.text

ENTRY(_brk)
	mov.l	Lminbrk, r0
	mov.l	@r0, r0
	cmp/hs	r4, r0
	bf	1f
	mov	r0, r4
1:
	mov.l	LSYS_break, r0
	trapa	#0x80
	bf	err
	xor	r0, r0
	mov.l	Lcurbrk, r1
	rts
	mov.l	r4, @r1
err:
	mov.l	Lcerror, r1
	jmp	@r1
	nop


	.align	2
Lminbrk:	.long	_C_LABEL(__minbrk)
LSYS_break:	.long	SYS_break
Lcurbrk:	.long	curbrk
Lcerror:	.long	cerror
