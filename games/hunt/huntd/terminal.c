/*	$NetBSD: terminal.c,v 1.7 2009/07/04 04:29:55 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: terminal.c,v 1.7 2009/07/04 04:29:55 dholland Exp $");
#endif /* not lint */

#include <stdarg.h>
#include "hunt.h"
#define TERM_WIDTH	80	/* Assume terminals are 80-char wide */

/*
 * cgoto:
 *	Move the cursor to the given position on the given player's
 *	terminal.
 */
void
cgoto(PLAYER *pp, int y, int x)
{
	if (x == pp->p_curx && y == pp->p_cury)
		return;
	sendcom(pp, MOVE, y, x);
	pp->p_cury = y;
	pp->p_curx = x;
}

/*
 * outch:
 *	Put out a single character.
 */
void
outch(PLAYER *pp, int ch)
{
	if (++pp->p_curx >= TERM_WIDTH) {
		pp->p_curx = 0;
		pp->p_cury++;
	}
	(void) putc(ch, pp->p_output);
}

/*
 * outstr:
 *	Put out a string of the given length.
 */
void
outstr(PLAYER *pp, const char *str, int len)
{
	pp->p_curx += len;
	pp->p_cury += (pp->p_curx / TERM_WIDTH);
	pp->p_curx %= TERM_WIDTH;
	while (len--)
		(void) putc(*str++, pp->p_output);
}

/*
 * clrscr:
 *	Clear the screen, and reset the current position on the screen.
 */
void
clrscr(PLAYER *pp)
{
	sendcom(pp, CLEAR);
	pp->p_cury = 0;
	pp->p_curx = 0;
}

/*
 * ce:
 *	Clear to the end of the line
 */
void
ce(PLAYER *pp)
{
	sendcom(pp, CLRTOEOL);
}

#if 0		/* XXX lukem */
/*
 * ref;
 *	Refresh the screen
 */
void
ref(PLAYER *pp)
{
	sendcom(pp, REFRESH);
}
#endif

/*
 * sendcom:
 *	Send a command to the given user
 */
void
sendcom(PLAYER *pp, int command, ...)
{
	va_list	ap;
	int arg1, arg2;

	va_start(ap, command);
	(void) putc(command, pp->p_output);
	switch (command & 0377) {
	case MOVE:
		arg1 = va_arg(ap, int);
		arg2 = va_arg(ap, int);
		(void) putc(arg1, pp->p_output);
		(void) putc(arg2, pp->p_output);
		break;
	case ADDCH:
	case READY:
		arg1 = va_arg(ap, int);
		(void) putc(arg1, pp->p_output);
		break;
	}

	va_end(ap);		/* No return needed for void functions. */
}
