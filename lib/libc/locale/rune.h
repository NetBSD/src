/*	$NetBSD: rune.h,v 1.3 2000/12/28 05:22:27 itojun Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *	@(#)rune.h	8.1 (Berkeley) 6/27/93
 */

#ifndef	_RUNE_H_
#define	_RUNE_H_

#include "runetype.h"
#include <stdio.h>

/* note the tree underlines! */
#define ___INVALID_RUNE(rl)	(rl)->__invalid_rune
#define ___mbrtowc(rl)		(rl)->__rune_mbrtowc
#define ___wcrtomb(rl)		(rl)->__rune_wcrtomb
#define ___CurrentRuneState(rl)	(rl)->__rune_RuneState
#define ___rune_initstate(rl)	___CurrentRuneState(rl)->__initstate
#define ___rune_sizestate(rl)	___CurrentRuneState(rl)->__sizestate
#define ___rune_packstate(rl)	___CurrentRuneState(rl)->__packstate
#define ___rune_unpackstate(rl)	___CurrentRuneState(rl)->__unpackstate

#define _INVALID_RUNE   	___INVALID_RUNE(_CurrentRuneLocale)
#define __mbrtowc		___mbrtowc(_CurrentRuneLocale)
#define __wcrtomb		___wcrtomb(_CurrentRuneLocale)
#define _CurrentRuneState	___CurrentRuneState(_CurrentRuneLocale)
#define __rune_initstate	___rune_initstate(_CurrentRuneLocale)
#define __rune_sizestate	___rune_sizestate(_CurrentRuneLocale)
#define __rune_packstate	___rune_packstate(_CurrentRuneLocale)
#define __rune_unpackstate	___rune_unpackstate(_CurrentRuneLocale)

#endif	/*! _RUNE_H_ */
