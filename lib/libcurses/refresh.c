/*	$NetBSD: refresh.c,v 1.31 2000/05/19 01:05:44 mycroft Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)refresh.c	8.7 (Berkeley) 8/13/94";
#else
__RCSID("$NetBSD: refresh.c,v 1.31 2000/05/19 01:05:44 mycroft Exp $");
#endif
#endif				/* not lint */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

/* the following is defined and set up in setterm.c */
extern struct tinfo *_cursesi_genbuf;

static int curwin = 0;
static short ly, lx;

static void	domvcur __P((int, int, int, int));
static int	makech __P((int));
static void	quickch __P((void));
static void	scrolln __P((int, int, int, int, int));

#ifndef _CURSES_USE_MACROS

/*
 * refresh --
 *	Make the current screen look like "stdscr" over the area covered by
 *	stdscr.
 */
int
refresh(void)
{
	return wrefresh(stdscr);
}

#endif

/*
 * wnoutrefresh --
 *	Add the contents of "win" to the virtual window.
 */
int
wnoutrefresh(WINDOW *win)
{
	short	wy, wx, y_off, x_off;

#ifdef DEBUG
	__CTRACE("wnoutrefresh: win %0.2o, flags 0x%08x\n", win, win->flags);
#endif

	if (curwin)
		return(OK);
	__virtscr->cury = win->cury + win->begy;
	__virtscr->curx = win->curx + win->begx;

	/* Copy the window flags from "win" to "__virtscr" */
	if (!(win->flags & __FULLWIN) && (win->flags & __CLEAROK))
		win->flags &= ~__CLEAROK;
	__virtscr->flags &= ~__LEAVEOK;
	__virtscr->flags |= win->flags;
	if (win->flags & __CLEAROK)
		win->flags &= ~__CLEAROK;

	for (wy = 0, y_off = win->begy; wy < win->maxy &&
	    y_off < __virtscr->maxy; wy++, y_off++) {
#ifdef DEBUG
		__CTRACE("wnoutrefresh: wy %d\tf: %d\tl:%d\tflags %x\n", wy,
		    *win->lines[wy]->firstchp,
		    *win->lines[wy]->lastchp,
		    win->lines[wy]->flags);
#endif
		y_off = wy + win->begy;
		if (*win->lines[wy]->firstchp <= win->maxx + win->ch_off &&
		    *win->lines[wy]->lastchp >= win->ch_off) {
			/* Copy line from "win" to "__virtscr". */
			for (wx = 0, x_off = win->begx; wx < win->maxx &&
			    x_off < __virtscr->maxx; wx++, x_off++) {
				__virtscr->lines[y_off]->line[x_off].attr =
				    win->lines[wy]->line[wx].attr;
				if (!(win->lines[wy]->line[wx].attr & __COLOR)
				    && (win->lines[wy]->line[wx].battr &
				    __COLOR))
					__virtscr->lines[y_off]->line[x_off].
					    attr |=
					    win->lines[wy]->line[wx].battr &
					    __COLOR;
				if (win->lines[wy]->line[wx].ch == ' ' &&
				    win->lines[wy]->line[wx].bch != ' ')
					__virtscr->lines[y_off]->line[x_off].ch
					    = win->lines[wy]->line[wx].bch;
				else
					__virtscr->lines[y_off]->line[x_off].ch
					    = win->lines[wy]->line[wx].ch;
			}

			/* Set flags on "__virtscr" and unset on "win". */
			if (win->lines[wy]->flags & __FORCEPAINT) {
				__virtscr->lines[y_off]->flags |= __FORCEPAINT;
				win->lines[wy]->flags &= ~__FORCEPAINT;
			}
			if (win->lines[wy]->flags & __ISPASTEOL)
				__virtscr->lines[y_off]->flags |= __ISPASTEOL;
			else
				__virtscr->lines[y_off]->flags &= ~__ISPASTEOL;
			if (win->lines[wy]->flags & __ISDIRTY)
				__virtscr->lines[y_off]->flags |= __ISDIRTY;

#ifdef DEBUG
			__CTRACE("win: firstch = %d, lastch = %d\n",
			    *win->lines[wy]->firstchp,
			    *win->lines[wy]->lastchp);
#endif
			/* Set change pointers on "__virtscr". */
			if (*__virtscr->lines[y_off]->firstchp >
			    *win->lines[wy]->firstchp + win->begx - win->ch_off)
				*__virtscr->lines[y_off]->firstchp =
				    *win->lines[wy]->firstchp + win->begx -
				    win->ch_off;
			if (*__virtscr->lines[y_off]->lastchp <
			    *win->lines[wy]->lastchp + win->begx - win->ch_off)
				*__virtscr->lines[y_off]->lastchp =
				    *win->lines[wy]->lastchp + win->begx -
				    win->ch_off;
#ifdef DEBUG
			__CTRACE("__virtscr: firstch = %d, lastch = %d\n",
			    *__virtscr->lines[y_off]->firstchp,
			    *__virtscr->lines[y_off]->lastchp);
#endif

			/* Set change pointers on "win". */
			if (*win->lines[wy]->firstchp >= win->ch_off)
				*win->lines[wy]->firstchp = win->maxx +
				    win->ch_off;
			if (*win->lines[wy]->lastchp < win->maxx + win->ch_off)
			*win->lines[wy]->lastchp = win->ch_off;
			if (*win->lines[wy]->lastchp <
			    *win->lines[wy]->firstchp) {
#ifdef DEBUG
				__CTRACE("wnoutrefresh: line %d notdirty\n",
				    wy);
#endif
				win->lines[wy]->flags &= ~__ISDIRTY;
			}
		}
	}

	return (OK);
}

/*
 * wrefresh --
 *	Make the current screen look like "win" over the area coverd by
 *	win.
 */
int
wrefresh(WINDOW *win)
{
	int	retval;

	curwin = (win == curscr);
	if (!curwin)
		retval = wnoutrefresh(win);
	else
		retval = OK;
	if (retval == OK) {
		retval = doupdate();
		if (!win->flags & __LEAVEOK) {
			win->cury = max(0, curscr->cury - win->begy);
			win->curx = max(0, curscr->curx - win->begx);
		}
	}
	curwin = 0;
	return(retval);
}

/*
 * doupdate --
 *	Make the current screen look like the virtual window "__virtscr".
 */
int
doupdate(void)
{
	WINDOW	*win;
	__LINE	*wlp;
	short	 wy;
	int	 dnum;

	/* Check if we need to restart ... */
	if (__endwin) {
		__endwin = 0;
		__restartwin();
	}

	if (curwin)
		win = curscr;
	else
		win = __virtscr;

	/* Initialize loop parameters. */
	ly = curscr->cury;
	lx = curscr->curx;
	wy = 0;

	if (!curwin)
		for (wy = 0; wy < win->maxy; wy++) {
			wlp = win->lines[wy];
			if (wlp->flags & __ISDIRTY)
				wlp->hash = __hash((char *)(void *)wlp->line,
				    (int) (win->maxx * __LDATASIZE));
		}

	if ((win->flags & __CLEAROK) || (curscr->flags & __CLEAROK) || curwin) {
		if (curscr->wattr & __COLOR)
			__unsetattr(0);
		tputs(CL, 0, __cputchar);
		ly = 0;
		lx = 0;
		if (!curwin) {
			curscr->flags &= ~__CLEAROK;
			curscr->cury = 0;
			curscr->curx = 0;
			werase(curscr);
		}
		__touchwin(win);
		win->flags &= ~__CLEAROK;
	}
	if (!CA) {
		if (win->curx != 0)
			putchar('\n');
		if (!curwin)
			werase(curscr);
	}
#ifdef DEBUG
	__CTRACE("doupdate: (%0.2o): curwin = %d\n", win, curwin);
	__CTRACE("doupdate: \tfirstch\tlastch\n");
#endif

	if (!curwin) {
		/*
		 * Invoke quickch() only if more than a quarter of the lines
		 * in the window are dirty.
		 */
		for (wy = 0, dnum = 0; wy < win->maxy; wy++)
			if (win->lines[wy]->flags &
			    (__ISDIRTY | __FORCEPAINT))
				dnum++;
		if (!__noqch && dnum > (int) win->maxy / 4)
			quickch();
	}

#ifdef DEBUG
	{
		int	i, j;

		__CTRACE("#####################################\n");
		for (i = 0; i < curscr->maxy; i++) {
			__CTRACE("C: %d:", i);
			__CTRACE(" 0x%x \n", curscr->lines[i]->hash);
			for (j = 0; j < curscr->maxx; j++)
				__CTRACE("%c", curscr->lines[i]->line[j].ch);
			__CTRACE("\n");
			__CTRACE(" attr:");
			for (j = 0; j < curscr->maxx; j++)
				__CTRACE(" %x", curscr->lines[i]->line[j].attr);
			__CTRACE("\n");
			__CTRACE("W: %d:", i);
			__CTRACE(" 0x%x \n", win->lines[i]->hash);
			__CTRACE(" 0x%x ", win->lines[i]->flags);
			for (j = 0; j < win->maxx; j++)
				__CTRACE("%c", win->lines[i]->line[j].ch);
			__CTRACE("\n");
			__CTRACE(" attr:");
			for (j = 0; j < win->maxx; j++)
				__CTRACE(" %x",
				    win->lines[i]->line[j].attr);
			__CTRACE("\n");
		}
	}
#endif				/* DEBUG */

	for (wy = 0; wy < win->maxy; wy++) {
/* XXX: remove this debug */
#ifdef DEBUG
		__CTRACE("doupdate: wy %d\tf: %d\tl:%d\tflags %x\n", wy,
		    *win->lines[wy]->firstchp,
		    *win->lines[wy]->lastchp,
		    win->lines[wy]->flags);
#endif
		if (!curwin)
			curscr->lines[wy]->hash = win->lines[wy]->hash;
		if (win->lines[wy]->flags & (__ISDIRTY | __FORCEPAINT)) {
			if (makech(wy) == ERR)
				return (ERR);
			else {
				if (*win->lines[wy]->firstchp >= 0)
					*win->lines[wy]->firstchp =
					    win->maxx;
				if (*win->lines[wy]->lastchp <
				    win->maxx)
					*win->lines[wy]->lastchp = 0;
				if (*win->lines[wy]->lastchp <
				    *win->lines[wy]->firstchp) {
#ifdef DEBUG
					__CTRACE("doupdate: line %d notdirty\n", wy);
#endif
					win->lines[wy]->flags &= ~__ISDIRTY;
				}
			}

		}
#ifdef DEBUG
		__CTRACE("\t%d\t%d\n", *win->lines[wy]->firstchp,
		    *win->lines[wy]->lastchp);
#endif
	}

#ifdef DEBUG
	__CTRACE("doupdate: ly=%d, lx=%d\n", ly, lx);
#endif

	if (curwin)
		domvcur(ly, lx, (int) win->cury, (int) win->curx);
	else {
		if (win->flags & __LEAVEOK) {
			curscr->cury = ly;
			curscr->curx = lx;
		} else {
			domvcur(ly, lx, win->cury, win->curx);
			curscr->cury = win->cury;
			curscr->curx = win->curx;
		}
	}

	/* Don't leave the screen with attributes set. */
	__unsetattr(0);
	(void) fflush(stdout);
	return (OK);
}

/*
 * makech --
 *	Make a change on the screen.
 */
static int
makech(wy)
	int	wy;
{
	WINDOW	*win;
	static __LDATA blank = {' ', 0};
	__LDATA *nsp, *csp, *cp, *cep;
	u_int	force;
	int	clsp, nlsp;	/* Last space in lines. */
	int	lch, wx;
	char	*ce, cm_buff[1024];
	attr_t	lspc;		/* Last space colour */
	attr_t	off, on;

#ifdef __GNUC__
	nlsp = lspc = 0;	/* XXX gcc -Wuninitialized */
#endif
	if (curwin)
		win = curscr;
	else
		win = __virtscr;
	/* Is the cursor still on the end of the last line? */
	if (wy > 0 && win->lines[wy - 1]->flags & __ISPASTEOL) {
		domvcur(ly, lx, ly + 1, 0);
		ly++;
		lx = 0;
	}
	wx = *win->lines[wy]->firstchp;
	if (wx < 0)
		wx = 0;
	else
		if (wx >= win->maxx)
			return (OK);
	lch = *win->lines[wy]->lastchp;
	if (lch < 0)
		return (OK);
	else
		if (lch >= (int) win->maxx)
			lch = win->maxx - 1;

	if (curwin)
		csp = &blank;
	else
		csp = &curscr->lines[wy]->line[wx];

	nsp = &win->lines[wy]->line[wx];
	force = win->lines[wy]->flags & __FORCEPAINT;
	win->lines[wy]->flags &= ~__FORCEPAINT;
	if (CE && !curwin) {
		cp = &win->lines[wy]->line[win->maxx - 1];
		lspc = cp->attr & __COLOR;
		while (cp->ch == ' ' && cp->attr == lspc)
			if (cp-- <= win->lines[wy]->line)
				break;
		nlsp = cp - win->lines[wy]->line;
	}
	if (!curwin)
		ce = CE;
	else
		ce = NULL;

	if (force) {
		if (CM) {
			t_goto(NULL, CM, lx, ly, cm_buff, 1023);
			tputs(cm_buff, 0, __cputchar);
		} else {
			tputs(HO, 0, __cputchar);
			__mvcur(0, 0, ly, lx, 1);
		}
	}

	while (wx <= lch) {
		if (!force && memcmp(nsp, csp, sizeof(__LDATA)) == 0) {
			if (wx <= lch) {
				while (wx <= lch &&
				    memcmp(nsp, csp, sizeof(__LDATA)) == 0) {
					nsp++;
					if (!curwin)
						++csp;
					++wx;
				}
				continue;
			}
			break;
		}
		domvcur(ly, lx, wy, wx);

#ifdef DEBUG
		__CTRACE("makech: 1: wx = %d, ly= %d, lx = %d, newy = %d, newx = %d, force =%d\n",
		    wx, ly, lx, wy, wx, force);
#endif
		ly = wy;
		lx = wx;
		while ((force || memcmp(nsp, csp, sizeof(__LDATA)) != 0)
		    && wx <= lch) {

			if (ce != NULL &&
			    wx >= nlsp && nsp->ch == ' ' && nsp->attr == lspc) {
				/* Check for clear to end-of-line. */
				cep = &curscr->lines[wy]->line[win->maxx - 1];
				while (cep->ch == ' ' && cep->attr == lspc)
					if (cep-- <= csp)
						break;
				clsp = cep - curscr->lines[wy]->line -
				    win->begx * __LDATASIZE;
#ifdef DEBUG
				__CTRACE("makech: clsp = %d, nlsp = %d\n",
				    clsp, nlsp);
#endif
				if (((clsp - nlsp >= strlen(CE) &&
				    clsp < win->maxx * __LDATASIZE) ||
				    wy == win->maxy - 1) &&
				    (!(lspc & __COLOR) ||
				    ((lspc & __COLOR) && UT))) {
					__unsetattr(0);
					if ((lspc & __COLOR) !=
					    (curscr->wattr & __COLOR)) {
						__set_color(lspc);
						curscr->wattr &= ~__COLOR;
						curscr->wattr |= lspc & __COLOR;
					}
					tputs(CE, 0, __cputchar);
					lx = wx + win->begx;
					while (wx++ <= clsp) {
						csp->ch = ' ';
						csp->attr = lspc;
						csp++;
					}
					return (OK);
				}
				ce = NULL;
			}

			/*
			 * Unset colour if appropriate.  Check to see
			 * if we also turn off standout, underscore and
			 * attributes.
			 */
			if (!(nsp->attr & __COLOR) &&
			    (curscr->wattr & __COLOR)) {
				if (OC != NULL && CC == NULL)
					tputs(OC, 0, __cputchar);
				if (OP != NULL) {
					tputs(OP, 0, __cputchar);
					curscr->wattr &= __mask_OP;
				}
			}

			off = ~nsp->attr & curscr->wattr;

			/*
			 * Unset attributes as appropriate.  Unset first
			 * so that the relevant attributes can be reset
			 * (because 'me' unsets 'mb', 'md', 'mh', 'mk',
			 * 'mp' and 'mr').  Check to see if we also turn off
			 * standout, attributes and colour.
			 */
			if (off & __TERMATTR && ME != NULL) {
				tputs(ME, 0, __cputchar);
				curscr->wattr &= __mask_ME;
				off &= __mask_ME;
			}

			/*
			 * Exit underscore mode if appropriate.
			 * Check to see if we also turn off standout,
			 * attributes and colour.
			 */
			if (off & __UNDERSCORE && UE != NULL) {
				tputs(UE, 0, __cputchar);
				curscr->wattr &= __mask_UE;
				off &= __mask_UE;
			}

			/*
			 * Exit standout mode as appropriate.
			 * Check to see if we also turn off underscore,
			 * attributes and colour.
			 * XXX
			 * Should use UC if SO/SE not available.
			 */
			if (off & __STANDOUT && SE != NULL) {
				tputs(SE, 0, __cputchar);
				curscr->wattr &= __mask_SE;
				off &= __mask_SE;
			}

			if (off & __ALTCHARSET && AE != NULL) {
				tputs(AE, 0, __cputchar);
				curscr->wattr &= ~__ALTCHARSET;
			}

			on = nsp->attr & ~curscr->wattr;

			/*
			 * Enter standout mode if appropriate.
			 */
			if (on & __STANDOUT && SO != NULL && SE != NULL) {
				tputs(SO, 0, __cputchar);
				curscr->wattr |= __STANDOUT;
			}

			/*
			 * Enter underscore mode if appropriate.
			 * XXX
			 * Should use UC if US/UE not available.
			 */
			if (on & __UNDERSCORE && US != NULL && UE != NULL) {
				tputs(US, 0, __cputchar);
				curscr->wattr |= __UNDERSCORE;
			}

			/*
			 * Set other attributes as appropriate.
			 */
			if (ME != NULL) {
				if (on & __BLINK && MB != NULL) {
					tputs(MB, 0, __cputchar);
					curscr->wattr |= __BLINK;
				}
				if (on & __BOLD && MD != NULL) {
					tputs(MD, 0, __cputchar);
					curscr->wattr |= __BOLD;
				}
				if (on & __DIM && MH != NULL) {
					tputs(MH, 0, __cputchar);
					curscr->wattr |= __DIM;
				}
				if (on & __BLANK && MK != NULL) {
					tputs(MK, 0, __cputchar);
					curscr->wattr |= __BLANK;
				}
				if (on & __PROTECT && MP != NULL) {
					tputs(MP, 0, __cputchar);
					curscr->wattr |= __PROTECT;
				}
				if (on & __REVERSE && MR != NULL) {
					tputs(MR, 0, __cputchar);
					curscr->wattr |= __REVERSE;
				}
			}

			/* Set/change colour as appropriate. */
			if ((nsp->attr & __COLOR) &&
			    cO != NULL && (OC != NULL || OP != NULL)) {
				if ((nsp->attr & __COLOR) !=
				    (curscr->wattr & __COLOR)) {
					__set_color(nsp->attr);
					curscr->wattr &= ~__COLOR;
					curscr->wattr |= nsp->attr &
					    __COLOR;
				}
			}

			/* Enter/exit altcharset mode as appropriate. */
			if (on & __ALTCHARSET && AS != NULL && AE != NULL) {
				tputs(AS, 0, __cputchar);
				curscr->wattr |= __ALTCHARSET;
			}

			wx++;
			if (wx >= win->maxx &&
			    wy == win->maxy - 1 && !curwin)
				if (win->flags & __SCROLLOK) {
					if (win->flags & __ENDLINE)
						__unsetattr(1);
					if (!(win->flags & __SCROLLWIN)) {
						if (!curwin) {
							csp->attr = nsp->attr;
							putchar((int)
							    (csp->ch =
							    nsp->ch));
						} else
							putchar((int) nsp->ch);
					}
					if (wx < curscr->maxx) {
						domvcur(ly, wx,
						    (int) (win->maxy - 1),
						    (int) (win->maxx - 1));
					}
					ly = win->maxy - 1;
					lx = win->maxx - 1;
					return (OK);
				}
			if (wx < win->maxx || wy < win->maxy - 1 ||
			    !(win->flags & __SCROLLWIN)) {
				if (!curwin) {
					csp->attr = nsp->attr;
					putchar((int) (csp->ch = nsp->ch));
					csp++;
				} else
					putchar((int) nsp->ch);
			}
#ifdef DEBUG
			__CTRACE("makech: putchar(%c)\n", nsp->ch & 0177);
#endif
			if (UC && ((nsp->attr & __STANDOUT) ||
			    (nsp->attr & __UNDERSCORE))) {
				putchar('\b');
				tputs(UC, 0, __cputchar);
			}
			nsp++;
#ifdef DEBUG
			__CTRACE("makech: 2: wx = %d, lx = %d\n", wx, lx);
#endif
		}
		if (lx == wx)	/* If no change. */
			break;
		lx = wx;
		if (lx >= COLS && AM)
			lx = COLS - 1;
		else
			if (wx >= win->maxx) {
				domvcur(ly, lx, ly,
				    (int) (win->maxx - 1));
				lx = win->maxx - 1;
			}
#ifdef DEBUG
		__CTRACE("makech: 3: wx = %d, lx = %d\n", wx, lx);
#endif
	}

	return (OK);
}

/*
 * domvcur --
 *	Do a mvcur, leaving attributes if necessary.
 */
static void
domvcur(oy, ox, ny, nx)
	int	oy, ox, ny, nx;
{
	__unsetattr(1);
	__mvcur(oy, ox, ny, nx, 1);
}

/*
 * Quickch() attempts to detect a pattern in the change of the window
 * in order to optimize the change, e.g., scroll n lines as opposed to
 * repainting the screen line by line.
 */

static void
quickch(void)
{
#define THRESH		(int) __virtscr->maxy / 4

	__LINE *clp, *tmp1, *tmp2;
	int	bsize, curs, curw, starts, startw, i, j;
	int	n, target, cur_period, bot, top, sc_region;
	__LDATA buf[1024];
	u_int	blank_hash;
	attr_t	bcolor;

#ifdef __GNUC__
	curs = curw = starts = startw = 0;	/* XXX gcc -Wuninitialized */
#endif
	/*
	 * Find how many lines from the top of the screen are unchanged.
	 */
	for (top = 0; top < __virtscr->maxy; top++)
		if (__virtscr->lines[top]->flags & __FORCEPAINT ||
		    __virtscr->lines[top]->hash != curscr->lines[top]->hash
		    || memcmp(__virtscr->lines[top]->line,
			curscr->lines[top]->line,
			(size_t) __virtscr->maxx * __LDATASIZE) != 0)
			break;
		else
			__virtscr->lines[top]->flags &= ~__ISDIRTY;
	/*
	 * Find how many lines from bottom of screen are unchanged.
	 */
	for (bot = __virtscr->maxy - 1; bot >= 0; bot--)
		if (__virtscr->lines[bot]->flags & __FORCEPAINT ||
		    __virtscr->lines[bot]->hash != curscr->lines[bot]->hash
		    || memcmp(__virtscr->lines[bot]->line,
			curscr->lines[bot]->line,
			(size_t) __virtscr->maxx * __LDATASIZE) != 0)
			break;
		else
			__virtscr->lines[bot]->flags &= ~__ISDIRTY;

	/*
	 * Work round an xterm bug where inserting lines causes all the
	 * inserted lines to be covered with the background colour we
	 * set on the first line (even if we unset it for subsequent
	 * lines).
	 */
	bcolor = __virtscr->lines[min(top,
	    __virtscr->maxy - 1)]->line[0].attr & __COLOR;
	for (i = top + 1, j = 0; i < bot; i++) {
		if ((__virtscr->lines[i]->line[0].attr & __COLOR) != bcolor) {
			bcolor = __virtscr->lines[i]->line[__virtscr->maxx].
			    attr & __COLOR;
			j = i - top;
		} else
			break;
	}
	top += j;

#ifdef NO_JERKINESS
	/*
	 * If we have a bottom unchanged region return.  Scrolling the
	 * bottom region up and then back down causes a screen jitter.
	 * This will increase the number of characters sent to the screen
	 * but it looks better.
	 */
	if (bot < __virtscr->maxy - 1)
		return;
#endif				/* NO_JERKINESS */

	/*
	 * Search for the largest block of text not changed.
	 * Invariants of the loop:
	 * - Startw is the index of the beginning of the examined block in
	 *   __virtscr.
	 * - Starts is the index of the beginning of the examined block in
	 *   curscr.
	 * - Curw is the index of one past the end of the exmined block in
	 *   __virtscr.
	 * - Curs is the index of one past the end of the exmined block in
	 *   curscr.
	 * - bsize is the current size of the examined block.
	*/

	for (bsize = bot - top; bsize >= THRESH; bsize--) {
		for (startw = top; startw <= bot - bsize; startw++)
			for (starts = top; starts <= bot - bsize;
			    starts++) {
				for (curw = startw, curs = starts;
				    curs < starts + bsize; curw++, curs++)
					if (__virtscr->lines[curw]->flags &
					    __FORCEPAINT ||
					    (__virtscr->lines[curw]->hash !=
						curscr->lines[curs]->hash ||
						memcmp(__virtscr->lines[curw]
						    ->line,
						    curscr->lines[curs]->line,
						    (size_t) __virtscr->maxx *
						    __LDATASIZE) != 0))
						break;
				if (curs == starts + bsize)
					goto done;
			}
	}
done:

	/* Did not find anything */
	if (bsize < THRESH)
		return;

#ifdef DEBUG
	__CTRACE("quickch:bsize=%d,starts=%d,startw=%d,curw=%d,curs=%d,top=%d,bot=%d\n",
	    bsize, starts, startw, curw, curs, top, bot);
#endif

	/*
	 * Make sure that there is no overlap between the bottom and top
	 * regions and the middle scrolled block.
	 */
	if (bot < curs)
		bot = curs - 1;
	if (top > starts)
		top = starts;

	n = startw - starts;

#ifdef DEBUG
	__CTRACE("#####################################\n");
	for (i = 0; i < curscr->maxy; i++) {
		__CTRACE("C: %d:", i);
		__CTRACE(" 0x%x \n", curscr->lines[i]->hash);
		for (j = 0; j < curscr->maxx; j++)
			__CTRACE("%c", curscr->lines[i]->line[j].ch);
		__CTRACE("\n");
		__CTRACE(" attr:");
		for (j = 0; j < curscr->maxx; j++)
			__CTRACE(" %x", curscr->lines[i]->line[j].attr);
		__CTRACE("\n");
		__CTRACE("W: %d:", i);
		__CTRACE(" 0x%x \n", __virtscr->lines[i]->hash);
		__CTRACE(" 0x%x ", __virtscr->lines[i]->flags);
		for (j = 0; j < __virtscr->maxx; j++)
			__CTRACE("%c", __virtscr->lines[i]->line[j].ch);
		__CTRACE("\n");
		__CTRACE(" attr:");
		for (j = 0; j < __virtscr->maxx; j++)
			__CTRACE(" %x", __virtscr->lines[i]->line[j].attr);
		__CTRACE("\n");
	}
#endif

	/* So we don't have to call __hash() each time */
	for (i = 0; i < __virtscr->maxx; i++) {
		buf[i].ch = ' ';
		buf[i].bch = ' ';
		buf[i].attr = 0;
		buf[i].battr = 0;
	}
	blank_hash = __hash((char *)(void *)buf,
	    (int) (__virtscr->maxx * __LDATASIZE));

	/*
	 * Perform the rotation to maintain the consistency of curscr.
	 * This is hairy since we are doing an *in place* rotation.
	 * Invariants of the loop:
	 * - I is the index of the current line.
	 * - Target is the index of the target of line i.
	 * - Tmp1 points to current line (i).
	 * - Tmp2 and points to target line (target);
	 * - Cur_period is the index of the end of the current period.
	 *   (see below).
	 *
	 * There are 2 major issues here that make this rotation non-trivial:
	 * 1.  Scrolling in a scrolling region bounded by the top
	 *     and bottom regions determined (whose size is sc_region).
	 * 2.  As a result of the use of the mod function, there may be a
	 *     period introduced, i.e., 2 maps to 4, 4 to 6, n-2 to 0, and
	 *     0 to 2, which then causes all odd lines not to be rotated.
	 *     To remedy this, an index of the end ( = beginning) of the
	 *     current 'period' is kept, cur_period, and when it is reached,
	 *     the next period is started from cur_period + 1 which is
	 *     guaranteed not to have been reached since that would mean that
	 *     all records would have been reached. (think about it...).
	 *
	 * Lines in the rotation can have 3 attributes which are marked on the
	 * line so that curscr is consistent with the visual screen.
	 * 1.  Not dirty -- lines inside the scrolled block, top region or
	 *                  bottom region.
	 * 2.  Blank lines -- lines in the differential of the scrolling
	 *		      region adjacent to top and bot regions
	 *                    depending on scrolling direction.
	 * 3.  Dirty line -- all other lines are marked dirty.
	 */
	sc_region = bot - top + 1;
	i = top;
	tmp1 = curscr->lines[top];
	cur_period = top;
	for (j = top; j <= bot; j++) {
		target = (i - top + n + sc_region) % sc_region + top;
		tmp2 = curscr->lines[target];
		curscr->lines[target] = tmp1;
		/* Mark block as clean and blank out scrolled lines. */
		clp = curscr->lines[target];
#ifdef DEBUG
		__CTRACE("quickch: n=%d startw=%d curw=%d i = %d target=%d ",
		    n, startw, curw, i, target);
#endif
		if ((target >= startw && target < curw) || target < top
		    || target > bot) {
#ifdef DEBUG
			__CTRACE("-- notdirty\n");
#endif
			__virtscr->lines[target]->flags &= ~__ISDIRTY;
		} else
			if ((n > 0 && target >= top && target < top + n) ||
			    (n < 0 && target <= bot && target > bot + n)) {
				if (clp->hash != blank_hash || memcmp(clp->line,
				    buf, (size_t) __virtscr->maxx * __LDATASIZE) !=0) {
					(void)memcpy(clp->line,  buf,
					    (size_t) __virtscr->maxx * __LDATASIZE);
#ifdef DEBUG
					__CTRACE("-- blanked out: dirty\n");
#endif
					clp->hash = blank_hash;
					__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1, 0);
				} else {
#ifdef DEBUG
					__CTRACE(" -- blank line already: dirty\n");
#endif
					__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1, 0);
				}
			} else {
#ifdef DEBUG
				__CTRACE(" -- dirty\n");
#endif
				__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1, 0);
			}
		if (target == cur_period) {
			i = target + 1;
			tmp1 = curscr->lines[i];
			cur_period = i;
		} else {
			tmp1 = tmp2;
			i = target;
		}
	}
#ifdef DEBUG
	__CTRACE("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
	for (i = 0; i < curscr->maxy; i++) {
		__CTRACE("C: %d:", i);
		for (j = 0; j < curscr->maxx; j++)
			__CTRACE("%c", curscr->lines[i]->line[j].ch);
		__CTRACE("\n");
		__CTRACE("W: %d:", i);
		for (j = 0; j < __virtscr->maxx; j++)
			__CTRACE("%c", __virtscr->lines[i]->line[j].ch);
		__CTRACE("\n");
	}
#endif
	if (n != 0)
		scrolln(starts, startw, curs, bot, top);
}

/*
 * scrolln --
 *	Scroll n lines, where n is starts - startw.
 */
static void /* ARGSUSED */
scrolln(starts, startw, curs, bot, top)
	int	starts, startw, curs, bot, top;
{
	int	i, oy, ox, n;

	oy = curscr->cury;
	ox = curscr->curx;
	n = starts - startw;

	/*
	 * XXX
	 * The initial tests that set __noqch don't let us reach here unless
	 * we have either CS + HO + SF/sf/SR/sr, or AL + DL.  SF/sf and SR/sr
	 * scrolling can only shift the entire scrolling region, not just a
	 * part of it, which means that the quickch() routine is going to be
	 * sadly disappointed in us if we don't have CS as well.
	 *
	 * If CS, HO and SF/sf are set, can use the scrolling region.  Because
	 * the cursor position after CS is undefined, we need HO which gives us
	 * the ability to move to somewhere without knowledge of the current
	 * location of the cursor.  Still call __mvcur() anyway, to update its
	 * idea of where the cursor is.
	 *
	 * When the scrolling region has been set, the cursor has to be at the
	 * last line of the region to make the scroll happen.
	 *
	 * Doing SF/SR or AL/DL appears faster on the screen than either sf/sr
	 * or al/dl, and, some terminals have AL/DL, sf/sr, and CS, but not
	 * SF/SR.  So, if we're scrolling almost all of the screen, try and use
	 * AL/DL, otherwise use the scrolling region.  The "almost all" is a
	 * shameless hack for vi.
	 */
	if (n > 0) {
		if (CS != NULL && HO != NULL && (SF != NULL ||
		    ((AL == NULL || DL == NULL ||
		    top > 3 || bot + 3 < __virtscr->maxy) && sf != NULL))) {
			tputs(__tscroll(CS, top, bot + 1), 0, __cputchar);
			__mvcur(oy, ox, 0, 0, 1);
			tputs(HO, 0, __cputchar);
			__mvcur(0, 0, bot, 0, 1);
			if (SF != NULL)
				tputs(__tscroll(SF, n, 0), 0, __cputchar);
			else
				for (i = 0; i < n; i++)
					tputs(sf, 0, __cputchar);
			tputs(__tscroll(CS, 0, (int) __virtscr->maxy), 0, __cputchar);
			__mvcur(bot, 0, 0, 0, 1);
			tputs(HO, 0, __cputchar);
			__mvcur(0, 0, oy, ox, 1);
			return;
		}

		/* Scroll up the block. */
		if (SF != NULL && top == 0) {
			__mvcur(oy, ox, bot, 0, 1);
			tputs(__tscroll(SF, n, 0), 0, __cputchar);
		} else
			if (DL != NULL) {
				__mvcur(oy, ox, top, 0, 1);
				tputs(__tscroll(DL, n, 0), 0, __cputchar);
			} else
				if (dl != NULL) {
					__mvcur(oy, ox, top, 0, 1);
					for (i = 0; i < n; i++)
						tputs(dl, 0, __cputchar);
				} else
					if (sf != NULL && top == 0) {
						__mvcur(oy, ox, bot, 0, 1);
						for (i = 0; i < n; i++)
							tputs(sf, 0, __cputchar);
					} else
						abort();

		/* Push down the bottom region. */
		__mvcur(top, 0, bot - n + 1, 0, 1);
		if (AL != NULL)
			tputs(__tscroll(AL, n, 0), 0, __cputchar);
		else
			if (al != NULL)
				for (i = 0; i < n; i++)
					tputs(al, 0, __cputchar);
			else
				abort();
		__mvcur(bot - n + 1, 0, oy, ox, 1);
	} else {
		/*
		 * !!!
		 * n < 0
		 *
		 * If CS, HO and SR/sr are set, can use the scrolling region.
		 * See the above comments for details.
		 */
		if (CS != NULL && HO != NULL && (SR != NULL ||
		    ((AL == NULL || DL == NULL ||
		    top > 3 || bot + 3 < __virtscr->maxy) && sr != NULL))) {
			tputs(__tscroll(CS, top, bot + 1), 0, __cputchar);
			__mvcur(oy, ox, 0, 0, 1);
			tputs(HO, 0, __cputchar);
			__mvcur(0, 0, top, 0, 1);

			if (SR != NULL)
				tputs(__tscroll(SR, -n, 0), 0, __cputchar);
			else
				for (i = n; i < 0; i++)
					tputs(sr, 0, __cputchar);
			tputs(__tscroll(CS, 0, (int) __virtscr->maxy), 0, __cputchar);
			__mvcur(top, 0, 0, 0, 1);
			tputs(HO, 0, __cputchar);
			__mvcur(0, 0, oy, ox, 1);
			return;
		}

		/* Preserve the bottom lines. */
		__mvcur(oy, ox, bot + n + 1, 0, 1);
		if (SR != NULL && bot == __virtscr->maxy)
			tputs(__tscroll(SR, -n, 0), 0, __cputchar);
		else
			if (DL != NULL)
				tputs(__tscroll(DL, -n, 0), 0, __cputchar);
			else
				if (dl != NULL)
					for (i = n; i < 0; i++)
						tputs(dl, 0, __cputchar);
				else
					if (sr != NULL && bot == __virtscr->maxy)
						for (i = n; i < 0; i++)
							tputs(sr, 0, __cputchar);
					else
						abort();

		/* Scroll the block down. */
		__mvcur(bot + n + 1, 0, top, 0, 1);
		if (AL != NULL)
			tputs(__tscroll(AL, -n, 0), 0, __cputchar);
		else
			if (al != NULL)
				for (i = n; i < 0; i++)
					tputs(al, 0, __cputchar);
			else
				abort();
		__mvcur(top, 0, oy, ox, 1);
	}
}

/*
 * __unsetattr --
 *	Unset attributes on curscr.  Leave standout, attribute and colour
 *	modes if necessary (!MS).  Always leave altcharset (xterm at least
 *	ignores a cursor move if we don't).
 */
void /* ARGSUSED */
__unsetattr(int checkms)
{
	int	isms;

	if (checkms)
		if (!MS) {
			isms = 1;
		} else {
			isms = 0;
		}
	else
		isms = 1;
#ifdef DEBUG
	__CTRACE("__unsetattr: checkms = %d, MS = %s, wattr = %08x\n",
	    checkms, MS ? "TRUE" : "FALSE", curscr->wattr);
#endif
		
	/*
         * Don't leave the screen in standout mode (check against MS).  Check
	 * to see if we also turn off underscore, attributes and colour.
	 */
	if (curscr->wattr & __STANDOUT && isms) {
		tputs(SE, 0, __cputchar);
		curscr->wattr &= __mask_SE;
	}
	/*
	 * Don't leave the screen in underscore mode (check against MS).
	 * Check to see if we also turn off attributes.  Assume that we
	 * also turn off colour.
	 */
	if (curscr->wattr & __UNDERSCORE && isms) {
		tputs(UE, 0, __cputchar);
		curscr->wattr &= __mask_UE;
	}
	/*
	 * Don't leave the screen with attributes set (check against MS).
	 * Assume that also turn off colour.
	 */
	if (curscr->wattr & __TERMATTR && isms) {
		tputs(ME, 0, __cputchar);
		curscr->wattr &= __mask_ME;
	}
	/* Don't leave the screen with altcharset set (don't check MS). */
	if (curscr->wattr & __ALTCHARSET) {
		tputs(AE, 0, __cputchar);
		curscr->wattr &= ~__ALTCHARSET;
	}
	/* Don't leave the screen with colour set (check against MS). */
	if (curscr->wattr & __COLOR && isms) {
		if (OC != NULL && CC == NULL)
			tputs(OC, 0, __cputchar);
		if (OP != NULL) {
			tputs(OP, 0, __cputchar);
			curscr->wattr &= __mask_OP;
		}
	}
}
