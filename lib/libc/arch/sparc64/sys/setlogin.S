/*	$NetBSD: setlogin.S,v 1.4 2002/05/07 01:34:22 eeh Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 * from: Header: setlogin.s,v 1.1 91/07/06 13:06:00 torek Exp
 */

#include <machine/asm.h>
#if defined(SYSLIBC_SCCS) && !defined(lint)
#if 0
	.asciz "@(#)setlogin.s	8.1 (Berkeley) 6/4/93"
#else
	RCSID("$NetBSD: setlogin.S,v 1.4 2002/05/07 01:34:22 eeh Exp $")
#endif
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"

	.globl	_C_LABEL(__logname_valid)	/* in getlogin() */

SYSCALL(setlogin)
#ifdef PIC
#ifdef BIGPIC
	PIC_PROLOGUE(%o1,%o2)
	set	_C_LABEL(__logname_valid), %o2
	ldx	[%o1 + %o2], %o1
	retl
	 st	%g0, [%o1]
#else
	PIC_PROLOGUE(%o1,%o2)
	ldx	[%o1 + _C_LABEL(__logname_valid)], %o1
	retl
	 st	%g0, [%o1]
#endif
#else
	/* Dunno what model we're using so we need to use full addresses */
	sethi	%hh(_C_LABEL(__logname_valid)), %o1
	sethi	%lm(_C_LABEL(__logname_valid)), %o2
	or	%o1, %hm(_C_LABEL(__logname_valid)), %o1
	or	%o2, %lo(_C_LABEL(__logname_valid)), %o2
	sllx	%o1, 32, %o1
	retl
	 st	%g0, [%o1 + %o2]
#endif
