/*	$NetBSD: copywin.c,v 1.9.6.1 2005/07/24 00:50:33 snj Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: copywin.c,v 1.9.6.1 2005/07/24 00:50:33 snj Exp $");
#endif				/* not lint */

#include <ctype.h>
#include <string.h>
#include "curses.h"
#include "curses_private.h"

/*
 * copywin --
 *     Copy the box starting at (sminrow, smincol) with a size that
 *     matches the destination box (dminrow, dmincol) by (dmaxrow, dmaxcol)
 *     from the source window srcwin to the destination window dstwin.
 *     All these coordindinates are relative to the relevant window.
 *     If dooverlay is true then the copy is nondestructive otherwise the
 *     copy is destructive.
 */
int copywin(const WINDOW *srcwin, WINDOW *dstwin,
	    int sminrow, int smincol,
	    int dminrow, int dmincol, int dmaxrow, int dmaxcol, int dooverlay)
{
	int dcol;
	__LDATA *sp, *end;

	/* overwrite() and overlay() can come here with -ve srcwin coords */
	if (sminrow < 0) {
		dminrow -= sminrow;
		sminrow = 0;
	}
	if (smincol < 0) {
		dmincol -= smincol;
		smincol = 0;
	}

	/* for symmetry allow dstwin coords to be -ve as well */
	if (dminrow < 0) {
		sminrow -= dminrow;
		dminrow = 0;
	}
	if (dmincol < 0) {
		smincol -= dmincol;
		dmincol = 0;
	}

	/* Bound dmaxcol for both windows (should be ok for dstwin) */
	if (dmaxcol >= dstwin->maxx)
		dmaxcol = dstwin->maxx - 1;
	if (smincol + (dmaxcol - dmincol) >= srcwin->maxx)
		dmaxcol = srcwin->maxx + dmincol - smincol - 1;
	if (dmaxcol < dmincol)
		/* nothing in the intersection */
		return OK;

	/* Bound dmaxrow for both windows (should be ok for dstwin) */
	if (dmaxrow >= dstwin->maxy)
		dmaxrow = dstwin->maxy - 1;
	if (sminrow + (dmaxrow - dminrow) >= srcwin->maxy)
		dmaxrow = srcwin->maxy + dminrow - sminrow - 1;

#ifdef DEBUG
	__CTRACE("copywin %s mode: from (%d,%d) to (%d,%d-%d,%d)\n",
		 dooverlay ? "overlay" : "overwrite",
		 sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol);
#endif

	for (; dminrow <= dmaxrow; sminrow++, dminrow++) {
		sp = &srcwin->lines[sminrow]->line[smincol];
		end = sp + dmaxcol - dmincol;
		if (dooverlay) {
			for (dcol = dmincol; sp <= end; dcol++, sp++) {
				/* XXX: Perhaps this should check for the
				 * background character
				 */
				if (!isspace(sp->ch)) {
					wmove(dstwin, dminrow, dcol);
					__waddch(dstwin, sp);
				}
			}
		} else {
			wmove(dstwin, dminrow, dmincol);
			for (; sp <= end; sp++) {
				__waddch(dstwin, sp);
			}
		}
	}
	return OK;
}

