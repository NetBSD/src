/*	$NetBSD: setlogin.S,v 1.5.4.1 1999/12/27 18:29:34 wrstuden Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 */

#include "SYS.h"

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
	RCSID("from: @(#)setlogin.s	5.1 (Berkeley) 5/6/91")
#else
	RCSID("$NetBSD: setlogin.S,v 1.5.4.1 1999/12/27 18:29:34 wrstuden Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

.globl	_C_LABEL(__logname_valid)		/* in getlogin() */

SYSCALL(setlogin)
#ifdef	PIC
#ifdef	__ELF__
	lea	%pc@(_GLOBAL_OFFSET_TABLE_@GOTPC),%a1
	movl	%a1@(_C_LABEL(__logname_valid)@GOT:w),%a1
#else
	movl	#_C_LABEL(_GLOBAL_OFFSET_TABLE_),%a1
	lea	%pc@(0,%a1:l),%a1
	movl	%a1@(_C_LABEL(__logname_valid):w),%a1
#endif
	clrl	%a1@
#else
	clrl	_C_LABEL(__logname_valid)
#endif
	rts				/* setlogin(name) */
