/*	$NetBSD: copywin.c,v 1.1 2000/04/18 12:23:01 blymn Exp $	*/

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
 *    derived from this software withough specific prior written permission
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

#include <ctype.h>
#include "curses.h"
#include "curses_private.h"

/*
 * copywin --
 *     Copy the box starting at (sminrow, smincol) with a size that
 *     matches the destination box (dminrow, dmincol) by (dmaxrow, dmaxcol)
 *     from the source window srcwin to the destination window dstwin.
 *     If overlay is true then the copy is nondestructive otherwise the
 *     copy is destructive.
 */
int copywin(const WINDOW *srcwin, WINDOW *dstwin, int sminrow,
            int smincol, int dminrow, int dmincol, int dmaxrow, 
            int dmaxcol, int overlay)
{
	int starty, startx, endy, endx, x, y, y1, y2, smaxrow, smaxcol;
	__LDATA *sp, *end;
	
	smaxrow = sminrow + dmaxrow - dminrow;
	smaxcol = smincol + dmaxcol - dmincol;
	starty = max(sminrow, dminrow);
	startx = max(smincol, dmincol);
	endy = min(sminrow + smaxrow, dminrow + dmaxrow);
	endx = min(smincol + smaxcol, dmincol + dmaxcol);
	if (starty >= endy || startx >= endx)
		return (OK);
	
	if (overlay == TRUE) {
		  /* non destructive copy */
#ifdef DEBUG
		__CTRACE("copywin overlay mode: from (%d,%d) to (%d,%d)\n",
			 starty, startx, endy, endx);
#endif
		y1 = starty - sminrow;
		y2 = starty - dminrow;
		for (y = starty; y < endy; y++, y1++, y2++) {
			end = &srcwin->lines[y1]->line[endx - srcwin->begx];
			x = startx - dstwin->begx;
			for (sp = &srcwin->lines[y1]->line[startx - srcwin->begx];
			     sp < end; sp++) {
				if (!isspace(sp->ch)) {
					wmove(dstwin, y2, x);
					__waddch(dstwin, sp);
				}
				x++;
			}
		}
		return (OK);
	} else {
		  /* destructive copy */
#ifdef DEBUG
		__CTRACE("copywin overwrite mode: from (%d,%d) to (%d,%d)\n",
			 starty, startx, endy, endx);
#endif
		x = endx - startx;
		for (y = starty; y < endy; y++) {
			(void) memcpy(
				&dstwin->lines[y - dstwin->begy]->line[startx - dstwin->begx],
				&srcwin->lines[y - srcwin->begy]->line[startx - srcwin->begx],
				(size_t) x * __LDATASIZE);
			__touchline(dstwin, y, (int) (startx - dstwin->begx),
				    (int) (endx - dstwin->begx), 0);
		}
		return (OK);
	}
}

