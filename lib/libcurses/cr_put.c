/*	$NetBSD: cr_put.c,v 1.25 2007/05/28 15:01:54 blymn Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)cr_put.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: cr_put.c,v 1.25 2007/05/28 15:01:54 blymn Exp $");
#endif
#endif				/* not lint */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

#define	HARDTABS	8

/*
 * Terminal driving and line formatting routines.  Basic motion optimizations
 * are done here as well as formatting lines (printing of control characters,
 * line numbering and the like).
 */

/* Stub function for the users. */
int
mvcur(int ly, int lx, int y, int x)
{
	return (__mvcur(ly, lx, y, x, 0));
}

static void fgoto __P((int));
static int plod __P((int, int));
static int plodput __P((int));
static int tabcol __P((int, int));

static int outcol, outline, destcol, destline;

/*
 * Sync the position of the output cursor.  Most work here is rounding for
 * terminal boundaries getting the column position implied by wraparound or
 * the lack thereof and rolling up the screen to get destline on the screen.
 */
int
__mvcur(int ly, int lx, int y, int x, int in_refresh)
{
#ifdef DEBUG
	__CTRACE(__CTRACE_OUTPUT,
	    "mvcur: moving cursor from (%d, %d) to (%d, %d)\n", ly, lx, y, x);
#endif
	destcol = x;
	destline = y;
	outcol = lx;
	outline = ly;
	fgoto(in_refresh);
	return (OK);
}

static void
fgoto(in_refresh)
	int	 in_refresh;
{
	int	 c, l;
	char   cgp[128];

#ifdef DEBUG
	__CTRACE(__CTRACE_OUTPUT, "fgoto: in_refresh=%d\n", in_refresh);
#endif /* DEBUG */
	if (destcol >= COLS) {
		destline += destcol / COLS;
		destcol %= COLS;
	}
	if (outcol >= COLS) {
		l = (outcol + 1) / COLS;
		outline += l;
		outcol %= COLS;
		if (__tc_am == 0) {
			while (l > 0) {
				if (__pfast) {
					if (__tc_cr)
						tputs(__tc_cr, 0, __cputchar);
					else
						__cputchar('\r');
				}
				if (__tc_nl)
					tputs(__tc_nl, 0, __cputchar);
				else
					__cputchar('\n');
				l--;
			}
			outcol = 0;
		}
		if (outline > LINES - 1) {
			destline -= outline - (LINES - 1);
			outline = LINES - 1;
		}
	}
	if (destline >= LINES) {
		l = destline;
		destline = LINES - 1;
		if (outline < LINES - 1) {
			c = destcol;
			if (__pfast == 0 && !__CA)
				destcol = 0;
			fgoto(in_refresh);
			destcol = c;
		}
		while (l >= LINES) {
			/* The following linefeed (or simulation thereof) is
			 * supposed to scroll up the screen, since we are on
			 * the bottom line.  We make the assumption that
			 * linefeed will scroll.  If ns is in the capability
			 * list this won't work.  We should probably have an
			 * sc capability but sf will generally take the place
			 * if it works.
			 *
			 * Superbee glitch: in the middle of the screen have to
			 * use esc B (down) because linefeed screws up in
			 * "Efficient Paging" (what a joke) mode (which is
			 * essential in some SB's because CRLF mode puts
			 * garbage in at end of memory), but you must use
			 * linefeed to scroll since down arrow won't go past
			 * memory end. I turned this off after recieving Paul
			 * Eggert's Superbee description which wins better. */
			if (__tc_nl /* && !__tc_xb */ && __pfast)
				tputs(__tc_nl, 0, __cputchar);
			else
				__cputchar('\n');
			l--;
			if (__pfast == 0)
				outcol = 0;
		}
	}
	if (destline < outline && !(__CA || __tc_up))
		destline = outline;
	if (__CA && t_goto(NULL, __tc_cm, destcol, destline, cgp,
	    sizeof(cgp) - 1) != -1) {
		/*
		 * Need this condition due to inconsistent behavior
		 * of backspace on the last column.
		 */
#ifdef DEBUG
		__CTRACE(__CTRACE_OUTPUT, "fgoto: cgp=%s\n", cgp);
#endif /* DEBUG */
		if (outcol != COLS - 1 &&
		    plod((int) strlen(cgp), in_refresh) > 0)
			plod(0, in_refresh);
		else
			tputs(cgp, 0, __cputchar);
	} else
		plod(0, in_refresh);
	outline = destline;
	outcol = destcol;
}
/*
 * Move (slowly) to destination.
 * Hard thing here is using home cursor on really deficient terminals.
 * Otherwise just use cursor motions, hacking use of tabs and overtabbing
 * and backspace.
 *
 * XXX this needs to be revisited for wide characters since we may output
 * XXX more than one byte for a character.
 */

static int plodcnt, plodflg;

static int
plodput(c)
	int	 c;
{
	if (plodflg)
		--plodcnt;
	else
		__cputchar(c);
	return (0);
}

static int
plod(cnt, in_refresh)
	int	 cnt, in_refresh;
{
	int	 i, j, k, soutcol, soutline;

#ifdef DEBUG
	__CTRACE(__CTRACE_OUTPUT, "plod: cnt=%d, in_refresh=%d\n",
	    cnt, in_refresh);
#endif /* DEBUG */
	plodcnt = plodflg = cnt;
	soutcol = outcol;
	soutline = outline;
	/*
	 * Consider homing and moving down/right from there, vs. moving
	 * directly with local motions to the right spot.
	 */
	if (__tc_ho) {
		/*
		 * i is the cost to home and tab/space to the right to get to
		 * the proper column.  This assumes nd space costs 1 char.  So
		 * i + destcol is cost of motion with home.
		 */
		if (__GT)
			i = (destcol / HARDTABS) + (destcol % HARDTABS);
		else
			i = destcol;

		/* j is cost to move locally without homing. */
		if (destcol >= outcol) {	/* if motion is to the right */
			j = destcol / HARDTABS - outcol / HARDTABS;
			if (__GT && j)
				j += destcol % HARDTABS;
			else
				j = destcol - outcol;
		} else
			/* leftward motion only works if we can backspace. */
			if (outcol - destcol <= i && (__tc_bs || __tc_bc))
				/* Cheaper to backspace. */
				i = j = outcol - destcol;
			else
				/* Impossibly expensive. */
				j = i + 1;

		/* k is the absolute value of vertical distance. */
		k = outline - destline;
		if (k < 0)
			k = -k;
		j += k;

		/* Decision.  We may not have a choice if no up. */
		if (i + destline < j || (!__tc_up && destline < outline)) {
			/*
			 * Cheaper to home.  Do it now and pretend it's a
			 * regular local motion.
			 */
			tputs(__tc_ho, 0, plodput);
			outcol = outline = 0;
		} else
			if (__tc_ll) {
				/*
				 * Quickly consider homing down and moving from
				 * there.  Assume cost of ll is 2.
				 */
				k = (LINES - 1) - destline;
				if (i + k + 2 < j && (k <= 0 || __tc_up)) {
					tputs(__tc_ll, 0, plodput);
					outcol = 0;
					outline = LINES - 1;
				}
			}
	} else
		/* No home and no up means it's impossible. */
		if (!__tc_up && destline < outline)
			return (-1);
	if (__GT)
		i = destcol % HARDTABS + destcol / HARDTABS;
	else
		i = destcol;
#ifdef notdef
	if (__tc_bt && outcol > destcol &&
	    (j = (((outcol + 7) & ~7) - destcol - 1) >> 3)) {
		j *= (k = strlen(__tc_bt));
		if ((k += (destcol & 7)) > 4)
			j += 8 - (destcol & 7);
		else
			j += k;
	} else
#endif
		j = outcol - destcol;

	/*
	 * If we will later need a \n which will turn into a \r\n by the
	 * system or the terminal, then don't bother to try to \r.
	 */
	if ((__NONL || !__pfast) && outline < destline)
		goto dontcr;

	/*
	 * If the terminal will do a \r\n and there isn't room for it, then
	 * we can't afford a \r.
	 */
	if (__tc_nc && outline >= destline)
		goto dontcr;

	/*
	 * If it will be cheaper, or if we can't back up, then send a return
	 * preliminarily.
	 */
	if (j > i + 1 || (outcol > destcol && !__tc_bs && !__tc_bc)) {
		/*
		 * BUG: this doesn't take the (possibly long) length of cr
		 * into account.
		 */
		if (__tc_cr)
			tputs(__tc_cr, 0, plodput);
		else
			plodput('\r');
		if (__tc_nc) {
			if (__tc_nl)
				tputs(__tc_nl, 0, plodput);
			else
				plodput('\n');
			outline++;
		}
		outcol = 0;
	}
dontcr:while (outline < destline) {
		outline++;
		if (__tc_nl)
			tputs(__tc_nl, 0, plodput);
		else
			plodput('\n');
		if (plodcnt < 0)
			goto out;
		if (__NONL || __pfast == 0)
			outcol = 0;
	}
	if (__tc_bt)
		k = (int) strlen(__tc_bt);
	while (outcol > destcol) {
		if (plodcnt < 0)
			goto out;
#ifdef notdef
		if (__tc_bt && outcol - destcol > k + 4) {
			tputs(__tc_bt, 0, plodput);
			outcol--;
			outcol &= ~7;
			continue;
		}
#endif
		outcol--;
		if (__tc_bc)
			tputs(__tc_bc, 0, plodput);
		else
			plodput('\b');
	}
	while (outline > destline) {
		outline--;
		tputs(__tc_up, 0, plodput);
		if (plodcnt < 0)
			goto out;
	}
	if (__GT && destcol - outcol > 1) {
		for (;;) {
			i = tabcol(outcol, HARDTABS);
			if (i > destcol)
				break;
			if (__tc_ta)
				tputs(__tc_ta, 0, plodput);
			else
				plodput('\t');
			outcol = i;
		}
		if (destcol - outcol > 4 && i < COLS && (__tc_bc || __tc_bs)) {
			if (__tc_ta)
				tputs(__tc_ta, 0, plodput);
			else
				plodput('\t');
			outcol = i;
			while (outcol > destcol) {
				outcol--;
				if (__tc_bc)
					tputs(__tc_bc, 0, plodput);
				else
					plodput('\b');
			}
		}
	}
	while (outcol < destcol) {
		/*
		 * Move one char to the right.  We don't use nd space because
		 * it's better to just print the char we are moving over.
		 */
		if (in_refresh)
			if (plodflg)	/* Avoid a complex calculation. */
				plodcnt--;
			else {
#ifndef HAVE_WCHAR
				i = curscr->lines[outline]->line[outcol].ch
				    & __CHARTEXT;
				if (curscr->lines[outline]->line[outcol].attr
				    == curscr->wattr)
					__cputchar(i);
#else
				if ((curscr->lines[outline]->line[outcol].attr
				    & WA_ATTRIBUTES)
				    == curscr->wattr ) {
					if (WCOL(curscr->lines[outline]->line[outcol]) > 0) {
						__cputwchar(curscr->lines[outline]->line[outcol].ch);
						__cursesi_putnsp(curscr->lines[outline]->line[outcol].nsp,
								outline,
								outcol);
#ifdef DEBUG
						__CTRACE(__CTRACE_OUTPUT,
						    "plod: (%d,%d)WCOL(%d), "
						    "putwchar(%x)\n",
						    outline, outcol,
						    WCOL(curscr->lines[outline]->line[outcol]),
						    curscr->lines[outline]->line[outcol].ch);
#endif /* DEBUG */
					}
				}
#endif /* HAVE_WCHAR */
				else
					goto nondes;
			}
		else
	nondes:	if (__tc_nd)
				tputs(__tc_nd, 0, plodput);
			else
				plodput(' ');
		outcol++;
		if (plodcnt < 0)
			goto out;
	}

out:	if (plodflg) {
		outcol = soutcol;
		outline = soutline;
	}
#ifdef DEBUG
	__CTRACE(__CTRACE_OUTPUT, "plod: returns %d\n", plodcnt);
#endif /* DEBUG */
	return (plodcnt);
}
/*
 * Return the column number that results from being in column col and
 * hitting a tab, where tabs are set every ts columns.  Work right for
 * the case where col > COLS, even if ts does not divide COLS.
 */
static int
tabcol(col, ts)
	int	 col, ts;
{
	int	 offset;

	if (col >= COLS) {
		offset = COLS * (col / COLS);
		col -= offset;
	} else
		offset = 0;
	return (col + ts - (col % ts) + offset);
}
