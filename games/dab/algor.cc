/*	$NetBSD: algor.cc,v 1.3 2006/05/14 03:20:42 christos Exp $	*/

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
 * algor.C: Computer algorithm
 */
#include "defs.h"
RCSID("$NetBSD: algor.cc,v 1.3 2006/05/14 03:20:42 christos Exp $")

#include "algor.h"
#include "board.h"
#include "box.h"
#include "random.h"

ALGOR::ALGOR(const char c) : PLAYER(c)
{
#ifdef notyet
    // Single Edges = (x + y) * 2
    _edge1 = (_b.nx() * _b.ny()) * 2;
    // Shared Edges = (x * (y - 1)) + ((x - 1) * y)
    _edge2 = (_b.nx() * (_b.ny() - 1)) + ((_b.nx() - 1) * _b.ny());
    // Maximum Edges filled before closure = x * y * 2
    _maxedge = _b.nx() * _b.ny() * 2;
#endif
}

// Find the first closure, i.e. a box that has 3 edges
int ALGOR::find_closure(size_t& y, size_t& x, int& dir, BOARD& b)
{
    RANDOM rdy(b.ny()), rdx(b.nx());

    for (y = rdy(); y < b.ny(); y = rdy()) {
	rdx.clear();
	for (x = rdx(); x < b.nx(); x = rdx()) {
	    BOX box(y, x, b);
	    if (box.count() == 3) {
		for (dir = BOX::first; dir < BOX::last; dir++)
		    if (!box.isset(dir))
			return 1;
		b.abort("find_closure: 3 sided box[%d,%d] has no free sides",
			y, x);
	    }
	}
    }
    return 0;
}

#if 0
size_t ALGOR::find_single()
{
    size_t ne;

    // Find the number of single edges in use
    for (size_t x = 0; x < b.nx(); x++) {
	BOX tbox(0, x, b);
	ne += tbox.isset(BOX::top);
	BOX bbox(b.ny() - 1, x, b);
	ne += bbox.isset(BOX::bottom);
    }
    for (size_t y = 0; y < _b.ny(); y++) {
	BOX lbox(y, 0, b);
	ne += lbox.isset(BOX::left);
	BOX rbox(y,_b.nx() - 1, b);
	ne += rbox.isset(BOX::right);
    }
    return ne;
}
#endif


// Count a closure, by counting all boxes that we can close in the current
// move
size_t ALGOR::count_closure(size_t& y, size_t& x, int& dir, BOARD& b)
{
    size_t i = 0;
    size_t tx, ty;
    int tdir, mv;

    while (find_closure(ty, tx, tdir, b)) {
	if (i == 0) {
	    // Mark the beginning of the closure
	    x = tx;
	    y = ty;
	    dir = tdir;
	}
	if ((mv = b.domove(ty, tx, tdir, getWho())) == -1)
	    b.abort("count_closure: Invalid move (%d, %d, %d)", y, x, dir);
	else
	    i += mv;
    }
    return i;
}


/*
 * Find the largest closure, by closing all possible closures.
 * return the number of boxes closed in the maximum closure,
 * and the first box of the maximum closure in (x, y, dir)
 */
size_t ALGOR::find_max_closure(size_t& y, size_t& x, int& dir, const BOARD& b)
{
    BOARD nb(b);
    int maxdir = -1;
    size_t nbox, maxbox = 0;
    size_t maxx = ~0, maxy = ~0;
    size_t tx = 0, ty = 0;	/* XXX: GCC */
    int tdir = 0;		/* XXX: GCC */

    while ((nbox = count_closure(ty, tx, tdir, nb)) != 0)
	if (nbox > maxbox) {
	    // This closure is better, update max
	    maxbox = nbox;
	    maxx = tx;
	    maxy = ty;
	    maxdir = tdir;
	}

    // Return the max found
    y = maxy;
    x = maxx;
    dir = maxdir;
    return maxbox;
}


// Find if a turn does not result in a capture on the given box
// and return the direction if found.
int ALGOR::try_good_turn(BOX& box, size_t y, size_t x, int& dir, BOARD& b)
{
    // Sanity check; we must have a good box
    if (box.count() >= 2)
	b.abort("try_good_turn: box[%d,%d] has more than 2 sides occupied",
		y, x);

    // Make sure we don't make a closure in an adjacent box.
    // We use a random direction to randomize the game
    RANDOM rd(BOX::last);
    for (dir = rd(); dir < BOX::last; dir = rd())
	if (!box.isset(dir)) {
	    size_t by = y + BOX::edges[dir].y;
	    size_t bx = x + BOX::edges[dir].x;
	    if (!b.bounds(by, bx))
		return 1;

	    BOX nbox(by, bx, b);
	    if (nbox.count() < 2)
		return 1;
	}

    return 0;
}


// Try to find a turn that does not result in an opponent closure, and
// return it in (x, y, dir); if not found return 0.
int ALGOR::find_good_turn(size_t& y, size_t& x, int& dir, const BOARD& b)
{
    BOARD nb(b);
    RANDOM rdy(b.ny()), rdx(b.nx());

    for (y = rdy(); y < b.ny(); y = rdy()) {
	rdx.clear();
	for (x = rdx(); x < b.nx(); x = rdx()) {
	    BOX box(y, x, nb);
	    if (box.count() < 2 && try_good_turn(box, y, x, dir, nb))
		return 1;
	}
    }
    return 0;
}

// On a box with 2 edges, return the first or the last free edge, depending
// on the order specified
int ALGOR::try_bad_turn(BOX& box, size_t& y, size_t& x, int& dir, BOARD& b,
			int last)
{
    if (4 - box.count() <= last)
	b.abort("try_bad_turn: Called at [%d,%d] for %d with %d",
		y, x, last, box.count());
    for (dir = BOX::first; dir < BOX::last; dir++)
	if (!box.isset(dir)) {
	    if (!last)
		return 1;
	    else
		last--;
	}
    return 0;
}

// Find a box that has 2 edges and return the first free edge of that
// box or the last free edge of that box
int ALGOR::find_bad_turn(size_t& y, size_t& x, int& dir, BOARD& b, int last)
{
    RANDOM rdy(b.ny()), rdx(b.nx());
    for (y = rdy(); y < b.ny(); y = rdy()) {
	rdx.clear();
	for (x = rdx(); x < b.nx(); x = rdx()) {
	    BOX box(y, x, b);
	    if ((4 - box.count()) > last &&
		try_bad_turn(box, y, x, dir, b, last))
		return 1;
	}
    }
    return 0;
}

size_t ALGOR::find_min_closure1(size_t& y, size_t& x, int& dir, const BOARD& b,
    int last)
{
    BOARD nb(b);
    int tdir, mindir = -1, mv;
    // number of boxes per closure
    size_t nbox, minbox = nb.nx() * nb.ny() + 1;
    size_t tx, ty, minx = ~0, miny = ~0;
    int xdir = 0;	/* XXX: GCC */

    while (find_bad_turn(ty, tx, tdir, nb, last)) {

        // Play a bad move that would cause the opponent's closure
	if ((mv = nb.domove(ty, tx, tdir, getWho())) != 0)
	    b.abort("find_min_closure1: Invalid move %d (%d, %d, %d)", mv,
		    ty, tx, tdir);

        // Count the opponent's closure
	if ((nbox = count_closure(y, x, xdir, nb)) == 0)
	    b.abort("find_min_closure1: no closure found");

	if (nbox <= minbox) {
	    // This closure has fewer boxes
	    minbox = nbox;
	    minx = tx;
	    miny = ty;
	    mindir = tdir;
	}
    }

    y = miny;
    x = minx;
    dir = mindir;
    return minbox;
}


// Search for the move that makes the opponent close the least number of
// boxes; returns 1 if a move found, 0 otherwise
size_t ALGOR::find_min_closure(size_t& y, size_t& x, int& dir, const BOARD& b)
{
    size_t x1, y1;
    int dir1;
    size_t count = b.ny() * b.nx() + 1, count1;

    for (size_t i = 0; i < 3; i++)
	if (count > (count1 = find_min_closure1(y1, x1, dir1, b, i))) {
	    count = count1;
	    y = y1;
	    x = x1;
	    dir = dir1;
	}

    return count != b.ny() * b.nx() + 1;
}

// Return a move in (y, x, dir)
void ALGOR::play(const BOARD& b, size_t& y, size_t& x, int& dir)
{
    // See if we can close the largest closure available
    if (find_max_closure(y, x, dir, b))
	return;

#ifdef notyet
    size_t sgl = find_single();
    size_t dbl = find_double();
#endif

    // See if we can play an edge without giving the opponent a box
    if (find_good_turn(y, x, dir, b))
	return;

    // Too bad, find the move that gives the opponent the fewer boxes
    if (find_min_closure(y, x, dir, b))
	return;
}
