/*	$NetBSD: gets.c,v 1.4.142.1 2016/07/09 20:24:57 skrll Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)gets.c	8.1 (Berkeley) 6/11/93
 */

#include "stand.h"

/*
 * This implementation assumes that getchar() does echo, because
 * on some machines, it is hard to keep echo from being done.
 * Those that need it can do echo in their getchar() function.
 *
 * Yes, the code below will echo CR, DEL, and other control chars,
 * but sending CR or DEL here is harmless.  All the other editing
 * characters will be followed by a newline, so it doesn't matter.
 * (Most terminals will not show them anyway.)
 */

void 
kgets(char *buf, size_t size)
{
	int c;
	char *lp;

top:
	lp = buf;

	for (;;) {
		if (lp - buf == size) {
			lp--;
			*lp = '\0';
			return;
		}
		c = getchar() & 0177;

#ifdef	GETS_MUST_ECHO	/* Preserved in case someone wants it... */
		putchar(c);
#endif

		switch (c) {

		default:
			*lp++ = c;
			continue;

		case '\177':
			putchar('\b');
			/* fall through */
		case '\b':
			putchar(' ');
			putchar('\b');
			/* fall through */
		case '#':
			if (lp > buf)
				lp--;
			continue;

#ifdef	GETS_REPRINT
		/*
		 * This is not very useful in a boot program.
		 * (It costs you 52 bytes on m68k, gcc -O3).
		 */
		case 'r'&037: {
			char *p;

			putchar('\n');
			for (p = buf; p < lp; ++p)
				putchar(*p);
			continue;
		}
#endif

		case '@':
		case 'u'&037:
		case 'w'&037:
			putchar('\n');
			goto top;

		case '\r':
			putchar('\n');
			/* fall through */
		case '\n':
			*lp = '\0';
			return;

		} /* switch */
	}
	/*NOTREACHED*/
}
