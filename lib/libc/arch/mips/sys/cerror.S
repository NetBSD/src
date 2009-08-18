/*	$NetBSD: cerror.S,v 1.13.46.2 2009/08/18 06:56:52 matt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
	RCSID("from: @(#)cerror.s	8.1 (Berkeley) 6/16/93")
#else
	RCSID("$NetBSD: cerror.S,v 1.13.46.2 2009/08/18 06:56:52 matt Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

#ifdef _REENTRANT
	.globl	_C_LABEL(__errno)
NESTED_NOPROFILE(__cerror, CALLFRAME_SIZ, ra)
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	PIC_PROLOGUE(__cerror)
	PTR_SUBU	sp, sp, CALLFRAME_SIZ
	PTR_S		ra, CALLFRAME_RA(sp)
	INT_S		v0, 3*SZREG(sp)		# save errno value

	SAVE_GP(CALLFRAME_GP)

	la		t9, _C_LABEL(__errno)	# locate address of errno
	jalr		t9

	INT_L		t0, 3*SZREG(sp)
	PTR_L		ra, CALLFRAME_RA(sp)
	INT_S		t0, 0(v0)		# update errno value
	PTR_ADDU	sp, sp, CALLFRAME_SIZ
	li		v0, -1
	li		v1, -1
	PIC_RETURN()
END(__cerror)
#else		
	.globl	_C_LABEL(errno)
LEAF_NOPROFILE(__cerror)
	PIC_PROLOGUE(__cerror)
	INT_S		v0, _C_LABEL(errno)
	li		v0, -1
	li		v1, -1
	PIC_RETURN()
END(__cerror)
#endif /* _REENTRANT */
