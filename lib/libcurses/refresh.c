/*	$NetBSD: refresh.c,v 1.54 2003/03/29 21:43:22 jdc Exp $	*/

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
__RCSID("$NetBSD: refresh.c,v 1.54 2003/03/29 21:43:22 jdc Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>
#include <string.h>

#include "curses.h"
#include "curses_private.h"

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
#ifdef DEBUG
	__CTRACE("wnoutrefresh: win %p\n", win);
#endif

	return _cursesi_wnoutrefresh(_cursesi_screen, win, 0, 0, win->begy,
	    win->begx, win->maxy, win->maxx);
}

/*
 * pnoutrefresh --
 *	Add the contents of "pad" to the virtual window.
 */
int
pnoutrefresh(WINDOW *pad, int pbegy, int pbegx, int sbegy, int sbegx,
    int smaxy, int smaxx)
{
	int pmaxy, pmaxx;

#ifdef DEBUG
	__CTRACE("pnoutrefresh: pad %p, flags 0x%08x\n", pad, pad->flags);
	__CTRACE("pnoutrefresh: (%d, %d), (%d, %d), (%d, %d)\n", pbegy, pbegx,
	    sbegy, sbegx, smaxy, smaxx);
#endif

	/* SUS says if these are negative, they should be treated as zero */
	if (pbegy < 0)
		pbegy = 0;
	if (pbegx < 0)
		pbegx = 0;
	if (sbegy < 0)
		sbegy = 0;
	if (sbegx < 0)
		sbegx = 0;

	/* Calculate rectangle on pad - used by _cursesi_wnoutrefresh */
	pmaxy = pbegy + smaxy - sbegy + 1;
	pmaxx = pbegx + smaxx - sbegx + 1;

	/* Check rectangle fits in pad */
	if (pmaxy > pad->maxy - pad->begy)
		pmaxy = pad->maxy - pad->begy;
	if (pmaxx > pad->maxx - pad->begx)
		pmaxx = pad->maxx - pad->begx;
		
	if (smaxy - sbegy < 0 || smaxx - sbegx < 0 )
		return ERR;

	return _cursesi_wnoutrefresh(_cursesi_screen, pad,
	    pad->begy + pbegy, pad->begx + pbegx, pad->begy + sbegy,
	    pad->begx + sbegx, pmaxy, pmaxx);
}

/*
 * _cursesi_wnoutrefresh --
 *      Does the grunt work for wnoutrefresh to the given screen.
 *	Copies the part of the window given by the rectangle
 *	(begy, begx) to (maxy, maxx) at screen position (wbegy, wbegx).
 */
int
_cursesi_wnoutrefresh(SCREEN *screen, WINDOW *win, int begy, int begx,
    int wbegy, int wbegx, int maxy, int maxx)
{

	short	wy, wx, y_off, x_off;
	__LINE	*wlp, *vlp;
	WINDOW	*sub_win;

#ifdef DEBUG
	__CTRACE("wnoutrefresh: win %p, flags 0x%08x\n", win, win->flags);
	__CTRACE("wnoutrefresh: (%d, %d), (%d, %d), (%d, %d)\n", begy, begx,
		wbegy, wbegx, maxy, maxx);
#endif

	if (screen->curwin)
		return(OK);

	/* Check that cursor position on "win" is valid for "__virtscr" */
	if (win->cury + wbegy - begy < screen->__virtscr->maxy &&
	    win->cury + wbegy - begy >= 0 && win->cury < maxy - begy)
		screen->__virtscr->cury = win->cury + wbegy - begy;
	if (win->curx + wbegx - begx < screen->__virtscr->maxx &&
	    win->curx + wbegx - begx >= 0 && win->curx < maxx - begx)
		screen->__virtscr->curx = win->curx + wbegx - begx;

	/* Copy the window flags from "win" to "__virtscr" */
	if (win->flags & __CLEAROK) {
		if (win->flags & __FULLWIN)
			screen->__virtscr->flags |= __CLEAROK;
		win->flags &= ~__CLEAROK;
	}
	screen->__virtscr->flags &= ~__LEAVEOK;
	screen->__virtscr->flags |= win->flags;

	for (wy = begy, y_off = wbegy; wy < maxy &&
	    y_off < screen->__virtscr->maxy; wy++, y_off++) {
 		wlp = win->lines[wy];
#ifdef DEBUG
		__CTRACE("wnoutrefresh: wy %d\tf: %d\tl:%d\tflags %x\n", wy,
		    *wlp->firstchp, *wlp->lastchp, wlp->flags);
#endif
		if ((wlp->flags & __ISDIRTY) == 0)
			continue;
		vlp = screen->__virtscr->lines[y_off];

		if (*wlp->firstchp < maxx + win->ch_off &&
		    *wlp->lastchp >= win->ch_off) {
			/* Copy line from "win" to "__virtscr". */
			for (wx = begx + *wlp->firstchp - win->ch_off,
			    x_off = wbegx + *wlp->firstchp - win->ch_off;
			    wx <= *wlp->lastchp && wx < maxx &&
			    x_off < screen->__virtscr->maxx; wx++, x_off++) {
				vlp->line[x_off].attr = wlp->line[wx].attr;
				/* Copy attributes */
				if (wlp->line[wx].attr & __COLOR)
					vlp->line[x_off].attr |=
					    wlp->line[wx].battr & ~__COLOR;
				else
					vlp->line[x_off].attr |=
					    wlp->line[wx].battr;
				/* Check for nca conflict with colour */
				if ((vlp->line[x_off].attr & __COLOR) &&
				    (vlp->line[x_off].attr &
				    _cursesi_screen->nca))
					vlp->line[x_off].attr &= ~__COLOR;
				/* Copy character */
				if (wlp->line[wx].ch == ' ' &&
				    wlp->line[wx].bch != ' ')
					vlp->line[x_off].ch
					    = wlp->line[wx].bch;
				else
					vlp->line[x_off].ch
					    = wlp->line[wx].ch;
			}

			/* Set flags on "__virtscr" and unset on "win". */
			if (wlp->flags & __ISPASTEOL)
				vlp->flags |= __ISPASTEOL;
			else
				vlp->flags &= ~__ISPASTEOL;
			if (wlp->flags & __ISDIRTY)
				vlp->flags |= __ISDIRTY;

#ifdef DEBUG
			__CTRACE("win: firstch = %d, lastch = %d\n",
			    *wlp->firstchp, *wlp->lastchp);
#endif
			/* Set change pointers on "__virtscr". */
			if (*vlp->firstchp >
			    *wlp->firstchp + wbegx - win->ch_off)
				*vlp->firstchp =
				    *wlp->firstchp + wbegx - win->ch_off;
			if (*vlp->lastchp <
			    *wlp->lastchp + wbegx - win->ch_off)
				*vlp->lastchp =
				    *wlp->lastchp + wbegx - win->ch_off;
#ifdef DEBUG
			__CTRACE("__virtscr: firstch = %d, lastch = %d\n",
			    *vlp->firstchp, *vlp->lastchp);
#endif
			/*
			 * Unset change pointers only if a window, as a pad
			 * can be displayed again without any of the contents
			 * changing.
			 */
			if (!(win->flags & __ISPAD)) {
				/* Set change pointers on "win". */
				if (*wlp->firstchp >= win->ch_off)
					*wlp->firstchp = maxx + win->ch_off;
				if (*wlp->lastchp < maxx + win->ch_off)
					*wlp->lastchp = win->ch_off;
				if (*wlp->lastchp < *wlp->firstchp) {
#ifdef DEBUG
					__CTRACE("wnoutrefresh: line %d notdirty\n", wy);
#endif
					wlp->flags &= ~__ISDIRTY;
				}
			}
		}
	}

	/* Recurse through any sub-windows */
	if ((sub_win = win->nextp) != win && sub_win != win->orig) {
#ifdef DEBUG
		__CTRACE("wnoutrefresh: win %o, sub_win %o\n", win, sub_win);
#endif
		return _cursesi_wnoutrefresh(screen, sub_win, 0, 0,
		    sub_win->begy, sub_win->begx,
		    sub_win->maxy, sub_win->maxx);
	} else
		return OK;
}

/*
 * wrefresh --
 *	Make the current screen look like "win" over the area covered by
 *	win.
 */
int
wrefresh(WINDOW *win)
{
	int retval;

#ifdef DEBUG
	__CTRACE("wrefresh: win %p\n", win);
#endif

	_cursesi_screen->curwin = (win == _cursesi_screen->curscr);
	if (!_cursesi_screen->curwin)
		retval = _cursesi_wnoutrefresh(_cursesi_screen, win, 0, 0,
		    win->begy, win->begx, win->maxy, win->maxx);
	else
		retval = OK;
	if (retval == OK) {
		retval = doupdate();
		if (!win->flags & __LEAVEOK) {
			win->cury = max(0, curscr->cury - win->begy);
			win->curx = max(0, curscr->curx - win->begx);
		}
	}
	_cursesi_screen->curwin = 0;
	return(retval);
}

 /*
 * prefresh --
 *	Make the current screen look like "pad" over the area coverd by
 *	the specified area of pad.
 */
int
prefresh(WINDOW *pad, int pbegy, int pbegx, int sbegy, int sbegx,
    int smaxy, int smaxx)
{
	int retval;

#ifdef DEBUG
	__CTRACE("prefresh: pad %p, flags 0x%08x\n", pad, pad->flags);
#endif

	/* Use pnoutrefresh() to avoid duplicating code here */
	retval = pnoutrefresh(pad, pbegy, pbegx, sbegy, sbegx, smaxy, smaxx);
	if (retval == OK) {
		retval = doupdate();
		if (!pad->flags & __LEAVEOK) {
			pad->cury = max(0, curscr->cury - pad->begy);
			pad->curx = max(0, curscr->curx - pad->begx);
		}
	}
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
	if (_cursesi_screen->endwin)
		__restartwin();

	if (_cursesi_screen->curwin)
		win = curscr;
	else
		win = _cursesi_screen->__virtscr;

	/* Initialize loop parameters. */
	_cursesi_screen->ly = curscr->cury;
	_cursesi_screen->lx = curscr->curx;
	wy = 0;

	if (!_cursesi_screen->curwin)
		for (wy = 0; wy < win->maxy; wy++) {
			wlp = win->lines[wy];
			if (wlp->flags & __ISDIRTY)
				wlp->hash = __hash((char *)(void *)wlp->line,
				    (size_t) (win->maxx * __LDATASIZE));
		}

	if ((win->flags & __CLEAROK) || (curscr->flags & __CLEAROK) ||
	    _cursesi_screen->curwin) {
		if (curscr->wattr & __COLOR)
			__unsetattr(0);
		tputs(__tc_cl, 0, __cputchar);
		_cursesi_screen->ly = 0;
		_cursesi_screen->lx = 0;
		if (!_cursesi_screen->curwin) {
			curscr->flags &= ~__CLEAROK;
			curscr->cury = 0;
			curscr->curx = 0;
			werase(curscr);
		}
		__touchwin(win);
		win->flags &= ~__CLEAROK;
	}
	if (!__CA) {
		if (win->curx != 0)
			__cputchar('\n');
		if (!_cursesi_screen->curwin)
			werase(curscr);
	}
#ifdef DEBUG
	__CTRACE("doupdate: (%p): curwin = %d\n", win,
		 _cursesi_screen->curwin);
	__CTRACE("doupdate: \tfirstch\tlastch\n");
#endif

	if (!_cursesi_screen->curwin) {
		/*
		 * Invoke quickch() only if more than a quarter of the lines
		 * in the window are dirty.
		 */
		for (wy = 0, dnum = 0; wy < win->maxy; wy++)
			if (win->lines[wy]->flags & __ISDIRTY)
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
				__CTRACE(" %x",
					 curscr->lines[i]->line[j].attr);
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
		wlp = win->lines[wy];
/* XXX: remove this debug */
#ifdef DEBUG
		__CTRACE("doupdate: wy %d\tf: %d\tl:%d\tflags %x\n", wy,
		    *wlp->firstchp, *wlp->lastchp, wlp->flags);
#endif
		if (!_cursesi_screen->curwin)
			curscr->lines[wy]->hash = wlp->hash;
		if (wlp->flags & __ISDIRTY) {
			if (makech(wy) == ERR)
				return (ERR);
			else {
				if (*wlp->firstchp >= 0)
					*wlp->firstchp = win->maxx;
				if (*wlp->lastchp < win->maxx)
					*wlp->lastchp = 0;
				if (*wlp->lastchp < *wlp->firstchp) {
#ifdef DEBUG
					__CTRACE("doupdate: line %d notdirty\n", wy);
#endif
					wlp->flags &= ~__ISDIRTY;
				}
			}

		}
#ifdef DEBUG
		__CTRACE("\t%d\t%d\n", *wlp->firstchp, *wlp->lastchp);
#endif
	}

#ifdef DEBUG
	__CTRACE("doupdate: ly=%d, lx=%d\n", _cursesi_screen->ly,
		 _cursesi_screen->lx);
#endif

	if (_cursesi_screen->curwin)
		domvcur(_cursesi_screen->ly, _cursesi_screen->lx,
			(int) win->cury, (int) win->curx);
	else {
		if (win->flags & __LEAVEOK) {
			curscr->cury = _cursesi_screen->ly;
			curscr->curx = _cursesi_screen->lx;
		} else {
			domvcur(_cursesi_screen->ly, _cursesi_screen->lx,
				win->cury, win->curx);
			curscr->cury = win->cury;
			curscr->curx = win->curx;
		}
	}

	/* Don't leave the screen with attributes set. */
	__unsetattr(0);
	(void) fflush(_cursesi_screen->outfd);
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
	static __LDATA blank = {' ', 0, ' ', 0};
	__LDATA *nsp, *csp, *cp, *cep;
	int	clsp, nlsp;	/* Last space in lines. */
	int	lch, wx;
	char	*ce;
	attr_t	lspc;		/* Last space colour */
	attr_t	off, on;

#ifdef __GNUC__
	nlsp = lspc = 0;	/* XXX gcc -Wuninitialized */
#endif
	if (_cursesi_screen->curwin)
		win = curscr;
	else
		win = __virtscr;
	/* Is the cursor still on the end of the last line? */
	if (wy > 0 && curscr->lines[wy - 1]->flags & __ISPASTEOL) {
		domvcur(_cursesi_screen->ly, _cursesi_screen->lx,
			_cursesi_screen->ly + 1, 0);
		_cursesi_screen->ly++;
		_cursesi_screen->lx = 0;
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

	if (_cursesi_screen->curwin)
		csp = &blank;
	else
		csp = &curscr->lines[wy]->line[wx];

	nsp = &win->lines[wy]->line[wx];
	if (__tc_ce && !_cursesi_screen->curwin) {
		cp = &win->lines[wy]->line[win->maxx - 1];
		lspc = cp->attr & __COLOR;
		while (cp->ch == ' ' && cp->attr == lspc)
			if (cp-- <= win->lines[wy]->line)
				break;
		nlsp = cp - win->lines[wy]->line;
		if (nlsp < 0)
			nlsp = 0;
	}
	if (!_cursesi_screen->curwin)
		ce = __tc_ce;
	else
		ce = NULL;

	while (wx <= lch) {
		if (memcmp(nsp, csp, sizeof(__LDATA)) == 0) {
			if (wx <= lch) {
				while (wx <= lch &&
				    memcmp(nsp, csp, sizeof(__LDATA)) == 0) {
					nsp++;
					if (!_cursesi_screen->curwin)
						++csp;
					++wx;
				}
				continue;
			}
			break;
		}
		domvcur(_cursesi_screen->ly, _cursesi_screen->lx, wy, wx);

#ifdef DEBUG
		__CTRACE("makech: 1: wx = %d, ly= %d, lx = %d, newy = %d, newx = %d\n",
		    wx, _cursesi_screen->ly, _cursesi_screen->lx, wy, wx);
#endif
		_cursesi_screen->ly = wy;
		_cursesi_screen->lx = wx;
		while (memcmp(nsp, csp, sizeof(__LDATA)) != 0 && wx <= lch) {
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
				if (((clsp - nlsp >= strlen(__tc_ce) &&
				    clsp < win->maxx * __LDATASIZE) ||
				    wy == win->maxy - 1) &&
				    (!(lspc & __COLOR) ||
				    ((lspc & __COLOR) && __tc_ut))) {
					__unsetattr(0);
					if (__using_color &&
					    ((lspc & __COLOR) !=
					    (curscr->wattr & __COLOR)))
						__set_color(curscr, lspc &
						    __COLOR);
					tputs(__tc_ce, 0, __cputchar);
					_cursesi_screen->lx = wx + win->begx;
					while (wx++ <= clsp) {
						csp->ch = ' ';
						csp->attr = lspc;
						csp++;
					}
					return (OK);
				}
				ce = NULL;
			}

#ifdef DEBUG
				__CTRACE("makech: have attributes %08x, need attributes %08x\n", curscr->wattr, nsp->attr);
#endif

			off = ~nsp->attr & curscr->wattr;

			/*
			 * Unset attributes as appropriate.  Unset first
			 * so that the relevant attributes can be reset
			 * (because 'me' unsets 'mb', 'md', 'mh', 'mk',
			 * 'mp' and 'mr').  Check to see if we also turn off
			 * standout, attributes and colour.
			 */
			if (off & __TERMATTR && __tc_me != NULL) {
				tputs(__tc_me, 0, __cputchar);
				curscr->wattr &= __mask_me;
				off &= __mask_me;
			}

			/*
			 * Exit underscore mode if appropriate.
			 * Check to see if we also turn off standout,
			 * attributes and colour.
			 */
			if (off & __UNDERSCORE && __tc_ue != NULL) {
				tputs(__tc_ue, 0, __cputchar);
				curscr->wattr &= __mask_ue;
				off &= __mask_ue;
			}

			/*
			 * Exit standout mode as appropriate.
			 * Check to see if we also turn off underscore,
			 * attributes and colour.
			 * XXX
			 * Should use uc if so/se not available.
			 */
			if (off & __STANDOUT && __tc_se != NULL) {
				tputs(__tc_se, 0, __cputchar);
				curscr->wattr &= __mask_se;
				off &= __mask_se;
			}

			if (off & __ALTCHARSET && __tc_ae != NULL) {
				tputs(__tc_ae, 0, __cputchar);
				curscr->wattr &= ~__ALTCHARSET;
			}

			/* Set/change colour as appropriate. */
			if (__using_color)
				__set_color(curscr, nsp->attr & __COLOR);

			on = nsp->attr & ~curscr->wattr;

			/*
			 * Enter standout mode if appropriate.
			 */
			if (on & __STANDOUT && __tc_so != NULL && __tc_se
			    != NULL) {
				tputs(__tc_so, 0, __cputchar);
				curscr->wattr |= __STANDOUT;
			}

			/*
			 * Enter underscore mode if appropriate.
			 * XXX
			 * Should use uc if us/ue not available.
			 */
			if (on & __UNDERSCORE && __tc_us != NULL &&
			    __tc_ue != NULL) {
				tputs(__tc_us, 0, __cputchar);
				curscr->wattr |= __UNDERSCORE;
			}

			/*
			 * Set other attributes as appropriate.
			 */
			if (__tc_me != NULL) {
				if (on & __BLINK && __tc_mb != NULL) {
					tputs(__tc_mb, 0, __cputchar);
					curscr->wattr |= __BLINK;
				}
				if (on & __BOLD && __tc_md != NULL) {
					tputs(__tc_md, 0, __cputchar);
					curscr->wattr |= __BOLD;
				}
				if (on & __DIM && __tc_mh != NULL) {
					tputs(__tc_mh, 0, __cputchar);
					curscr->wattr |= __DIM;
				}
				if (on & __BLANK && __tc_mk != NULL) {
					tputs(__tc_mk, 0, __cputchar);
					curscr->wattr |= __BLANK;
				}
				if (on & __PROTECT && __tc_mp != NULL) {
					tputs(__tc_mp, 0, __cputchar);
					curscr->wattr |= __PROTECT;
				}
				if (on & __REVERSE && __tc_mr != NULL) {
					tputs(__tc_mr, 0, __cputchar);
					curscr->wattr |= __REVERSE;
				}
			}

			/* Enter/exit altcharset mode as appropriate. */
			if (on & __ALTCHARSET && __tc_as != NULL &&
			    __tc_ae != NULL) {
				tputs(__tc_as, 0, __cputchar);
				curscr->wattr |= __ALTCHARSET;
			}

			wx++;
			if (wx >= win->maxx &&
			    wy == win->maxy - 1 && !_cursesi_screen->curwin)
				if (win->flags & __SCROLLOK) {
					if (win->flags & __ENDLINE)
						__unsetattr(1);
					if (!(win->flags & __SCROLLWIN)) {
						if (!_cursesi_screen->curwin) {
							csp->attr = nsp->attr;
							__cputchar((int)
							    (csp->ch =
							    nsp->ch));
						} else
							__cputchar((int) nsp->ch);
					}
					if (wx < curscr->maxx) {
						domvcur(_cursesi_screen->ly, wx,
						    (int) (win->maxy - 1),
						    (int) (win->maxx - 1));
					}
					_cursesi_screen->ly = win->maxy - 1;
					_cursesi_screen->lx = win->maxx - 1;
					return (OK);
				}
			if (wx < win->maxx || wy < win->maxy - 1 ||
			    !(win->flags & __SCROLLWIN)) {
				if (!_cursesi_screen->curwin) {
					csp->attr = nsp->attr;
					__cputchar((int) (csp->ch = nsp->ch));
					csp++;
				} else
					__cputchar((int) nsp->ch);
			}
#ifdef DEBUG
			__CTRACE("makech: putchar(%c)\n", nsp->ch & 0177);
#endif
			if (__tc_uc && ((nsp->attr & __STANDOUT) ||
			    (nsp->attr & __UNDERSCORE))) {
				__cputchar('\b');
				tputs(__tc_uc, 0, __cputchar);
			}
			nsp++;
#ifdef DEBUG
			__CTRACE("makech: 2: wx = %d, lx = %d\n", wx, _cursesi_screen->lx);
#endif
		}
		if (_cursesi_screen->lx == wx)	/* If no change. */
			break;
		_cursesi_screen->lx = wx;
		if (_cursesi_screen->lx >= COLS && __tc_am)
			_cursesi_screen->lx = COLS - 1;
		else
			if (wx >= win->maxx) {
				domvcur(_cursesi_screen->ly,
					_cursesi_screen->lx,
					_cursesi_screen->ly,
					(int) (win->maxx - 1));
				_cursesi_screen->lx = win->maxx - 1;
			}
#ifdef DEBUG
		__CTRACE("makech: 3: wx = %d, lx = %d\n", wx,
			 _cursesi_screen->lx);
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

static __LDATA buf[128];
static  u_int last_hash;
static  size_t last_hash_len;
#define BLANKSIZE (sizeof(buf) / sizeof(buf[0]))

static void
quickch(void)
{
#define THRESH		(int) __virtscr->maxy / 4

	__LINE *clp, *tmp1, *tmp2;
	int	bsize, curs, curw, starts, startw, i, j;
	int	n, target, cur_period, bot, top, sc_region;
	u_int	blank_hash;
	attr_t	bcolor;

#ifdef __GNUC__
	curs = curw = starts = startw = 0;	/* XXX gcc -Wuninitialized */
#endif
	/*
	 * Find how many lines from the top of the screen are unchanged.
	 */
	for (top = 0; top < __virtscr->maxy; top++)
		if (__virtscr->lines[top]->flags & __ISDIRTY &&
		    (__virtscr->lines[top]->hash != curscr->lines[top]->hash ||
		     memcmp(__virtscr->lines[top]->line,
			  curscr->lines[top]->line,
			  (size_t) __virtscr->maxx * __LDATASIZE) != 0))
			break;
		else
			__virtscr->lines[top]->flags &= ~__ISDIRTY;
	/*
	 * Find how many lines from bottom of screen are unchanged.
	 */
	for (bot = __virtscr->maxy - 1; bot >= 0; bot--)
		if (__virtscr->lines[bot]->flags & __ISDIRTY &&
		    (__virtscr->lines[bot]->hash != curscr->lines[bot]->hash ||
		     memcmp(__virtscr->lines[bot]->line,
			  curscr->lines[bot]->line,
			  (size_t) __virtscr->maxx * __LDATASIZE) != 0))
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
					if (__virtscr->lines[curw]->hash !=
					    curscr->lines[curs]->hash)
						break;
				if (curs != starts + bsize)
					continue;
				for (curw = startw, curs = starts;
				    curs < starts + bsize; curw++, curs++)
					if (memcmp(__virtscr->lines[curw]->line,
					    curscr->lines[curs]->line,
					    (size_t) __virtscr->maxx *
					    __LDATASIZE) != 0)
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

	if (buf[0].ch != ' ') {
		for (i = 0; i < BLANKSIZE; i++) {
			buf[i].ch = ' ';
			buf[i].bch = ' ';
			buf[i].attr = 0;
			buf[i].battr = 0;
		}
	}

	if (__virtscr->maxx != last_hash_len) {
		blank_hash = 0;
		for (i = __virtscr->maxx; i > BLANKSIZE; i -= BLANKSIZE) {
			blank_hash = __hash_more((char *)(void *)buf, sizeof(buf),
			    blank_hash);
		}
		blank_hash = __hash_more((char *)(void *)buf,
		    i * sizeof(buf[0]), blank_hash);
		/* cache result in static data - screen width doesn't change often */
		last_hash_len = __virtscr->maxx;
		last_hash = blank_hash;
	} else
		blank_hash = last_hash;

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
				    clp->line + 1, (__virtscr->maxx - 1) *
				    __LDATASIZE) || memcmp(clp->line, buf,
				    __LDATASIZE)) {
					for (i = __virtscr->maxx; i > BLANKSIZE;
					    i -= BLANKSIZE) {
						(void)memcpy(clp->line + i -
						    BLANKSIZE, buf, sizeof(buf));
					}
					(void)memcpy(clp->line , buf, i * 
					    sizeof(buf[0]));
#ifdef DEBUG
					__CTRACE("-- blanked out: dirty\n");
#endif
					clp->hash = blank_hash;
					__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1);
				} else {
#ifdef DEBUG
					__CTRACE(" -- blank line already: dirty\n");
#endif
					__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1);
				}
			} else {
#ifdef DEBUG
				__CTRACE(" -- dirty\n");
#endif
				__touchline(__virtscr, target, 0, (int) __virtscr->maxx - 1);
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
	 * we have either cs + ho + SF/sf/SR/sr, or AL + DL.  SF/sf and SR/sr
	 * scrolling can only shift the entire scrolling region, not just a
	 * part of it, which means that the quickch() routine is going to be
	 * sadly disappointed in us if we don't have cs as well.
	 *
	 * If cs, ho and SF/sf are set, can use the scrolling region.  Because
	 * the cursor position after cs is undefined, we need ho which gives us
	 * the ability to move to somewhere without knowledge of the current
	 * location of the cursor.  Still call __mvcur() anyway, to update its
	 * idea of where the cursor is.
	 *
	 * When the scrolling region has been set, the cursor has to be at the
	 * last line of the region to make the scroll happen.
	 *
	 * Doing SF/SR or AL/DL appears faster on the screen than either sf/sr
	 * or AL/DL, and, some terminals have AL/DL, sf/sr, and cs, but not
	 * SF/SR.  So, if we're scrolling almost all of the screen, try and use
	 * AL/DL, otherwise use the scrolling region.  The "almost all" is a
	 * shameless hack for vi.
	 */
	if (n > 0) {
		if (__tc_cs != NULL && __tc_ho != NULL && (__tc_SF != NULL ||
		    ((__tc_AL == NULL || __tc_DL == NULL ||
		    top > 3 || bot + 3 < __virtscr->maxy) &&
		    __tc_sf != NULL))) {
			tputs(__tscroll(__tc_cs, top, bot + 1), 0, __cputchar);
			__mvcur(oy, ox, 0, 0, 1);
			tputs(__tc_ho, 0, __cputchar);
			__mvcur(0, 0, bot, 0, 1);
			if (__tc_SF != NULL)
				tputs(__tscroll(__tc_SF, n, 0), 0, __cputchar);
			else
				for (i = 0; i < n; i++)
					tputs(__tc_sf, 0, __cputchar);
			tputs(__tscroll(__tc_cs, 0, (int) __virtscr->maxy), 0,
			    __cputchar);
			__mvcur(bot, 0, 0, 0, 1);
			tputs(__tc_ho, 0, __cputchar);
			__mvcur(0, 0, oy, ox, 1);
			return;
		}

		/* Scroll up the block. */
		if (__tc_SF != NULL && top == 0) {
			__mvcur(oy, ox, bot, 0, 1);
			tputs(__tscroll(__tc_SF, n, 0), 0, __cputchar);
		} else
			if (__tc_DL != NULL) {
				__mvcur(oy, ox, top, 0, 1);
				tputs(__tscroll(__tc_DL, n, 0), 0, __cputchar);
			} else
				if (__tc_dl != NULL) {
					__mvcur(oy, ox, top, 0, 1);
					for (i = 0; i < n; i++)
						tputs(__tc_dl, 0, __cputchar);
				} else
					if (__tc_sf != NULL && top == 0) {
						__mvcur(oy, ox, bot, 0, 1);
						for (i = 0; i < n; i++)
							tputs(__tc_sf, 0,
							    __cputchar);
					} else
						abort();

		/* Push down the bottom region. */
		__mvcur(top, 0, bot - n + 1, 0, 1);
		if (__tc_AL != NULL)
			tputs(__tscroll(__tc_AL, n, 0), 0, __cputchar);
		else
			if (__tc_al != NULL)
				for (i = 0; i < n; i++)
					tputs(__tc_al, 0, __cputchar);
			else
				abort();
		__mvcur(bot - n + 1, 0, oy, ox, 1);
	} else {
		/*
		 * !!!
		 * n < 0
		 *
		 * If cs, ho and SR/sr are set, can use the scrolling region.
		 * See the above comments for details.
		 */
		if (__tc_cs != NULL && __tc_ho != NULL && (__tc_SR != NULL ||
		    ((__tc_AL == NULL || __tc_DL == NULL || top > 3 ||
		    bot + 3 < __virtscr->maxy) && __tc_sr != NULL))) {
			tputs(__tscroll(__tc_cs, top, bot + 1), 0, __cputchar);
			__mvcur(oy, ox, 0, 0, 1);
			tputs(__tc_ho, 0, __cputchar);
			__mvcur(0, 0, top, 0, 1);

			if (__tc_SR != NULL)
				tputs(__tscroll(__tc_SR, -n, 0), 0, __cputchar);
			else
				for (i = n; i < 0; i++)
					tputs(__tc_sr, 0, __cputchar);
			tputs(__tscroll(__tc_cs, 0, (int) __virtscr->maxy), 0,
			    __cputchar);
			__mvcur(top, 0, 0, 0, 1);
			tputs(__tc_ho, 0, __cputchar);
			__mvcur(0, 0, oy, ox, 1);
			return;
		}

		/* Preserve the bottom lines. */
		__mvcur(oy, ox, bot + n + 1, 0, 1);
		if (__tc_SR != NULL && bot == __virtscr->maxy)
			tputs(__tscroll(__tc_SR, -n, 0), 0, __cputchar);
		else
			if (__tc_DL != NULL)
				tputs(__tscroll(__tc_DL, -n, 0), 0, __cputchar);
			else
				if (__tc_dl != NULL)
					for (i = n; i < 0; i++)
						tputs(__tc_dl, 0, __cputchar);
				else
					if (__tc_sr != NULL &&
					    bot == __virtscr->maxy)
						for (i = n; i < 0; i++)
							tputs(__tc_sr, 0,
							    __cputchar);
					else
						abort();

		/* Scroll the block down. */
		__mvcur(bot + n + 1, 0, top, 0, 1);
		if (__tc_AL != NULL)
			tputs(__tscroll(__tc_AL, -n, 0), 0, __cputchar);
		else
			if (__tc_al != NULL)
				for (i = n; i < 0; i++)
					tputs(__tc_al, 0, __cputchar);
			else
				abort();
		__mvcur(top, 0, oy, ox, 1);
	}
}

/*
 * __unsetattr --
 *	Unset attributes on curscr.  Leave standout, attribute and colour
 *	modes if necessary (!ms).  Always leave altcharset (xterm at least
 *	ignores a cursor move if we don't).
 */
void /* ARGSUSED */
__unsetattr(int checkms)
{
	int	isms;

	if (checkms)
		if (!__tc_ms) {
			isms = 1;
		} else {
			isms = 0;
		}
	else
		isms = 1;
#ifdef DEBUG
	__CTRACE("__unsetattr: checkms = %d, ms = %s, wattr = %08x\n",
	    checkms, __tc_ms ? "TRUE" : "FALSE", curscr->wattr);
#endif
		
	/*
         * Don't leave the screen in standout mode (check against ms).  Check
	 * to see if we also turn off underscore, attributes and colour.
	 */
	if (curscr->wattr & __STANDOUT && isms) {
		tputs(__tc_se, 0, __cputchar);
		curscr->wattr &= __mask_se;
	}
	/*
	 * Don't leave the screen in underscore mode (check against ms).
	 * Check to see if we also turn off attributes.  Assume that we
	 * also turn off colour.
	 */
	if (curscr->wattr & __UNDERSCORE && isms) {
		tputs(__tc_ue, 0, __cputchar);
		curscr->wattr &= __mask_ue;
	}
	/*
	 * Don't leave the screen with attributes set (check against ms).
	 * Assume that also turn off colour.
	 */
	if (curscr->wattr & __TERMATTR && isms) {
		tputs(__tc_me, 0, __cputchar);
		curscr->wattr &= __mask_me;
	}
	/* Don't leave the screen with altcharset set (don't check ms). */
	if (curscr->wattr & __ALTCHARSET) {
		tputs(__tc_ae, 0, __cputchar);
		curscr->wattr &= ~__ALTCHARSET;
	}
	/* Don't leave the screen with colour set (check against ms). */
	if (__using_color && isms)
		__unset_color(curscr);
}
