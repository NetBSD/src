/*	$NetBSD: callvec.c,v 1.18.22.1 2017/12/03 11:36:36 jdolecek Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 *
 *	@(#)callvec.c	8.1 (Berkeley) 6/10/93
 */

#include <machine/dec_prom.h>

#ifndef _LP64
const
#endif
struct callback callvec = {
	._strcat	= (void *)DEC_PROM_STRCAT,
	._strcmp	= (void *)DEC_PROM_STRCMP,
	._strcpy	= (void *)DEC_PROM_STRCPY,
	._strlen	= (void *)DEC_PROM_STRLEN,
	._getchar	= (void *)DEC_PROM_GETCHAR,
	._unsafe_gets	= (void *)DEC_PROM_GETS,
	._puts		= (void *)DEC_PROM_PUTS,
	._printf	= (void *)DEC_PROM_PRINTF,
	._setenv	= (void *)DEC_PROM_SETENV2,
	._getenv	= (void *)DEC_PROM_GETENV2,
	._unsetenv	= (void *)DEC_PROM_UNSETENV,
	._clear_cache	= (void *)DEC_PROM_CLEARCACHE,
};

const struct callback *callv = &callvec;
