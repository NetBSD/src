/*	$NetBSD: player.C,v 1.1.1.1 2003/12/26 17:57:03 christos Exp $	 */

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
 * player.C: Player base class
 */

#include "defs.h"
RCSID("$Id: player.C,v 1.1.1.1 2003/12/26 17:57:03 christos Exp $")

#include "board.h"
#include "player.h"

PLAYER::PLAYER(char who) :
    _who(who),
    _score(0),
    _total(0),
    _games(0),
    _ties(0)
{
}

void PLAYER::init(void)
{
    _score = 0;
}

void PLAYER::wl(size_t sc)
{
    _total += _score;
    _games += sc < _score;
    _ties += sc == _score;
}

int PLAYER::domove(BOARD& b)
{
    size_t y, x;
    int dir;
    int score;

    for (;;) {
	if (b.full())
	    return 0;

	play(b, y, x, dir);

	switch (score = b.domove(y, x, dir, _who)) {
	case 0:
	    // No closure
	    return 1;

	case -1:
	    // Not a valid move
	    b.bell();
	    break;

	default:
	    // Closure, play again
	    _score += score;
	    break;
	}
    }
}
