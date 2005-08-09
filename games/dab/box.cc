/*	$NetBSD: box.cc,v 1.2 2005/08/09 02:38:32 christos Exp $	*/

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
 * box.C: Box computations
 */
#include "defs.h"
RCSID("$NetBSD: box.cc,v 1.2 2005/08/09 02:38:32 christos Exp $")

#include "box.h"
#include "board.h"
#include "gamescreen.h"
#include <curses.h>

const POINT BOX::edges[BOX::last] =
    { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };
const POINT BOX::corners[BOX::last] =
    { { -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 } };
const int  BOX::syms[BOX::last] =
    { GAMESCREEN::GS_HLINE, GAMESCREEN::GS_HLINE,
      GAMESCREEN::GS_VLINE, GAMESCREEN::GS_VLINE };

BOX::BOX(size_t py, size_t px, BOARD& b) :
    _b(b)
{
    _centery = py * 2 + 1;
    _centerx = px * 2 + 1;
}

void BOX::addcorner(size_t y, size_t x)
{
    char sym;
    _b.getScrn()->moveto(y, x);
    if (x == 0) {
	if (y == 0)
	    sym = GAMESCREEN::GS_ULCORNER;
	else if (y == _b.ty() - 1)
	    sym = GAMESCREEN::GS_LLCORNER;
	else
	    sym = GAMESCREEN::GS_LTEE;
    } else if (x == _b.tx() - 1) {
	if (y == 0)
	    sym = GAMESCREEN::GS_URCORNER;
	else if (y == _b.ty() - 1)
	    sym = GAMESCREEN::GS_LRCORNER;
	else
	    sym = GAMESCREEN::GS_RTEE;
    } else if (y == 0)
	sym = GAMESCREEN::GS_TTEE;
    else if (y == _b.ty() - 1)
	sym = GAMESCREEN::GS_BTEE;
    else
	sym = GAMESCREEN::GS_PLUS;

    _b.getScrn()->addedge(sym);
}

// Paint a box
void BOX::paint(void)
{
    int e;
    if (_b.getScrn() == NULL)
	return;

    _b.getScrn()->moveto(_centery, _centerx);
    _b.getScrn()->addsym(name());

    for (e = BOX::first; e < BOX::last; e++) {
	addcorner(_centery + corners[e].y, _centerx + corners[e].x);
	_b.getScrn()->moveto(_centery + edges[e].y, _centerx + edges[e].x);
	_b.getScrn()->addedge(edge(static_cast<EDGE>(e)));
    }
    _b.getScrn()->redraw();
}

// Return the name
int& BOX::name(void)
{
    return _b.data(_centery, _centerx);
}

// Set an edge
void BOX::set(int e)
{
    _b.data(_centery + edges[e].y, _centerx + edges[e].x) = syms[e];
}

// Clear an edge
void BOX::clr(int e)
{
    _b.data(_centery + edges[e].y, _centerx + edges[e].x) = ' ';
}

// Test an edge
int BOX::isset(int e) const
{
    return _b.data(_centery + edges[e].y, _centerx + edges[e].x) != ' ';
}

// Return the edge
int& BOX::edge(int e)
{
    return _b.data(_centery + edges[e].y, _centerx + edges[e].x);
}

// Count the number of edges set in the box
int BOX::count(void) const
{
    int cnt = 0;

    for (int e = BOX::first; e < BOX::last; e++)
	cnt += isset(static_cast<EDGE>(e));
    return cnt;
}

// Clear the box
void BOX::reset(void)
{
    for (int e = BOX::first; e < BOX::last; e++)
	clr(static_cast<EDGE>(e));
    name() = ' ';
} 
