/*	$NetBSD: brk.S,v 1.16 2013/07/16 20:49:42 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 */

#include "SYS.h"

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
	RCSID("from: @(#)brk.s	5.1 (Berkeley) 5/12/90")
#else
	RCSID("$NetBSD: brk.S,v 1.16 2013/07/16 20:49:42 matt Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

	.globl	_end
	.globl	_C_LABEL(__minbrk)
	.globl	_C_LABEL(__curbrk)

#ifdef WEAK_ALIAS
WEAK_ALIAS(brk, _brk)
#endif

	.data
_C_LABEL(__minbrk):
	.long	_end

	.text

ENTRY(_brk)
#ifdef	PIC
	lea	(_GLOBAL_OFFSET_TABLE_@GOTPC,%pc),%a1
	movl	_C_LABEL(__minbrk)@GOT:w(%a1),%a0
	movl	(%a0),%d0
#else
	movl	_C_LABEL(__minbrk),%d0
#endif
	cmpl	4(%sp),%d0
	jls	ok
	movl	%d0,4(%sp)
ok:
	movl	#SYS_break,%d0
	trap	#0
	jcs	err
#ifdef	PIC
	movl	_C_LABEL(__curbrk)@GOT:w(%a1),%a0
	movl	4(%sp),(%a0)
#else
	movl	4(%sp),_C_LABEL(__curbrk)
#endif
	clrl	%d0
	rts
err:
	jra	PIC_PLT(CERROR)
