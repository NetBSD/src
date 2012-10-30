/*	$NetBSD: strpbrk.c,v 1.1.1.1.14.1 2012/10/30 18:55:18 yamt Exp $	*/

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "@(#)strpbrk.c	8.1 (Berkeley) 6/4/93";
static const char rcsid[] = "Id: strpbrk.c,v 1.2 2005/04/27 04:56:12 sra Exp ";
#endif /* LIBC_SCCS and not lint */

/*
 * Copyright (c) 1985, 1993
 *    The Regents of the University of California.  All rights reserved.
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
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
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

#include "port_before.h"

#include <sys/param.h>
#include <sys/cdefs.h>

#include <string.h>

#include "port_after.h"

#ifndef NEED_STRPBRK
int __strpbrk_unneeded__;
#else

/*%
 * Find the first occurrence in s1 of a character in s2 (excluding NUL).
 */
char *
strpbrk(const char *s1, const char *s2) {
	const char *scanp;
	int c, sc;

	while ((c = *s1++) != 0) {
		for (scanp = s2; (sc = *scanp++) != 0;)
			if (sc == c)
				return ((char *)(s1 - 1));
	}
	return (NULL);
}

#endif /*NEED_STRPBRK*/

/*! \file */
