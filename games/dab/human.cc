/*	$NetBSD: human.cc,v 1.2 2005/07/02 15:48:03 jdc Exp $	*/

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
 * human.C: Human interface for dots, using rogue-like keys.
 */
#include "defs.h"
RCSID("$NetBSD: human.cc,v 1.2 2005/07/02 15:48:03 jdc Exp $")

#include "human.h"
#include "board.h"
#include "box.h"
#include "ttyscrn.h"

#define CONTROL(a) ((a) & 037)

extern GAMESCREEN *sc;

HUMAN::HUMAN(const char c) :
    PLAYER(c),
    _curx(0),
    _cury(1)
{
}

void HUMAN::play(const BOARD& b, size_t& y, size_t& x, int& dir)
{
    int mv;
    b.setpos(_cury, _curx);

    for (;;) {
	switch (mv = b.getmove()) {
	case 'h': case 'H':
	    _curx -= 2;
	    break;

	case 'l': case 'L':
	    _curx += 2;
	    break;

	case 'k': case 'K':
	    _cury -= 2;
	    break;

	case 'j': case 'J':
	    _cury += 2;
	    break;

	case 'u': case 'U':
	    _curx += 1;
	    _cury -= 1;
	    break;

	case 'y': case 'Y':
	    _curx -= 1;
	    _cury -= 1;
	    break;

	case 'b': case 'B':
	    _curx -= 1;
	    _cury += 1;
	    break;

	case 'n': case 'N':
	    _curx += 1;
	    _cury += 1;
	    break;

	case 'q': case 'Q':
	    // Cleanup
	    delete sc;
	    exit(0);

	case CONTROL('L'): case CONTROL('R'):
	    b.clean();
	    b.paint();
	    break;

	case ' ':
	    {
		x = _curx / 2;
		y = _cury / 2;

		if (_cury & 1) {
		    if (_curx == 0)
			dir = BOX::left;
		    else {
			x--;
			dir = BOX::right;
		    }
		}

		if (_curx & 1) {
		    if (_cury == 0)
			dir = BOX::top;
		    else {
			y--;
			dir = BOX::bottom;
		    }
		}
	    }
	    return;

	default:
	    break;
	}

        // We add 2 before the comparison to avoid underflow
	if ((2 + _curx) - (_curx & 1) < 2)
	    _curx = (b.nx() * 2) + (_curx & 1);
	if (_curx >= (b.nx() * 2) + 1)
	    _curx = (_curx & 1);

	if ((2 + _cury) - (_cury & 1) < 2)
	    _cury = (b.ny() * 2) + (_cury & 1);
	if (_cury >= (b.ny() * 2) + 1)
	    _cury = (_cury & 1);

	b.setpos(_cury, _curx);
    }
}
