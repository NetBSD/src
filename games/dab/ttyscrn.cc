/*	$NetBSD: ttyscrn.cc,v 1.2 2003/12/27 18:24:51 martin Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ttyscrn.C: Curses screen implementation for dots
 */

#include "defs.h"
RCSID("$NetBSD: ttyscrn.cc,v 1.2 2003/12/27 18:24:51 martin Exp $")

#include <stdio.h>
#include <curses.h>
#include <sys/ioctl.h>

#include "player.h"
#include "ttyscrn.h"

void TTYSCRN::clean(void)
{
    clear();
}

void TTYSCRN::moveto(size_t y, size_t x)
{
    move(y + TTYSCRN::offsy, x + TTYSCRN::offsx);
}

void TTYSCRN::addsym(const int sym)
{
    addch(sym);
}

void TTYSCRN::addedge(const int sym)
{
    int nsym;
#ifdef A_ALTCHARSET
    if (_acs) {
	switch (sym) {
	case GS_HLINE:
	    nsym = ACS_HLINE;
	    break;
	case GS_VLINE:
	    nsym = ACS_VLINE;
	    break;
	case GS_ULCORNER:
	    nsym = ACS_ULCORNER;
	    break;
	case GS_URCORNER:
	    nsym = ACS_URCORNER;
	    break;
	case GS_LLCORNER:
	    nsym = ACS_LLCORNER;
	    break;
	case GS_LRCORNER:
	    nsym = ACS_LRCORNER;
	    break;
	case GS_LTEE:
	    nsym = ACS_LTEE;
	    break;
	case GS_RTEE:
	    nsym = ACS_RTEE;
	    break;
	case GS_TTEE:
	    nsym = ACS_TTEE;
	    break;
	case GS_BTEE:
	    nsym = ACS_BTEE;
	    break;
	case GS_PLUS:
	    nsym = ACS_PLUS;
	    break;
	case ' ':
	    addsym(' ');
	    return;
	default:
	    ::abort();
	}
	attron(A_ALTCHARSET);
	addch(nsym);
	attroff(A_ALTCHARSET);
	return;
    }
#endif
    switch (sym) {
    case GS_HLINE:
	nsym = '-';
	break;
    case GS_VLINE:
	nsym = '|';
	break;
    case GS_ULCORNER:
	nsym = '.';
	break;
    case GS_URCORNER:
	nsym = '.';
	break;
    case GS_LLCORNER:
	nsym = '.';
	break;
    case GS_LRCORNER:
	nsym = '.';
	break;
    case GS_LTEE:
	nsym = '.';
	break;
    case GS_RTEE:
	nsym = '.';
	break;
    case GS_TTEE:
	nsym = '.';
	break;
    case GS_BTEE:
	nsym = '.';
	break;
    case GS_PLUS:
	nsym = '+';
	break;
    case ' ':
	addsym(' ');
	return;
    default:
	::abort();
    }
    addsym(nsym);
}

void TTYSCRN::redraw(void)
{
    refresh();
    doupdate();
}

void TTYSCRN::bell(void)
{
    putc('\007', stdout);
}

int TTYSCRN::getinput(void)
{
    return getch();
}

void TTYSCRN::score(size_t s, const PLAYER& p)
{
    mvwprintw(stdscr, _sy + s + TTYSCRN::offsscore, _sx, "S %c:%5zd", p.getWho(),
	      p.getScore());
}

void TTYSCRN::total(size_t s, const PLAYER& p)
{
    mvwprintw(stdscr, _sy + s + TTYSCRN::offstotal, _sx, "T %c:%5zd", p.getWho(),
	      p.getTotal());
}

void TTYSCRN::games(size_t s, const PLAYER& p)
{
    mvwprintw(stdscr, _sy + s + TTYSCRN::offsgames, _sx, "G %c:%5zd", p.getWho(),
	      p.getGames());
}

void TTYSCRN::ties(const PLAYER& p)
{
    mvwprintw(stdscr, _sy + TTYSCRN::offsties, _sx, "G =:%5zd", p.getTies());
}

TTYSCRN* TTYSCRN::create(int acs, size_t y, size_t x)
{
    int tx, ty;

    initscr();

    tx = getmaxx(stdscr);
    ty = getmaxy(stdscr);

    if (tx == ERR || ty == ERR || (size_t)tx < x * 2 + TTYSCRN::offsx + 12
	|| (size_t)ty < y * 2 + TTYSCRN::offsy) {
	endwin();
	return NULL;
    }
    cbreak();
    noecho();


    TTYSCRN* that = new TTYSCRN;

    that->_tx = tx;
    that->_ty = ty;
    that->_sx = tx - 12;
    that->_sy = TTYSCRN::offsy;
    that->_acs = acs;

    return that;
}

TTYSCRN::~TTYSCRN(void)
{
    nocbreak();
    echo();
    endwin();
}
