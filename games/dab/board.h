/*	$NetBSD: board.h,v 1.3.4.1 2012/03/05 19:12:08 sborrill Exp $	*/

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
 * board.h: Board functions
 */

#ifndef _H_BOARD
#define _H_BOARD

#include <stdlib.h>

class GAMESCREEN;
class PLAYER;

class BOARD {
  public:
    // Constructors and destructor
    BOARD(size_t y, size_t x, GAMESCREEN* scrn);// For the main screen
    BOARD(const BOARD& b);			// For scratch screens
    ~BOARD();

    // member access
    size_t nx(void) const { return _nx; }
    size_t ny(void) const { return _ny; }
    size_t tx(void) const { return _tx; }
    size_t ty(void) const { return _ty; }
    GAMESCREEN* getScrn(void) const { return _scrn; }
    int& data(size_t y, size_t x) { return _b[y][x]; }

    // Computing
    int domove(size_t y, size_t x, int dir, char c);	// Play move
    void init(void);					// Initialize a new game
    int full(void) const;				// True if no more moves
    int bounds(size_t y, size_t x) const;		// True if in bounds

    // Screen updates
    void paint(void) const;				// Redraw screen
    void clean(void) const;				// Clear screen
    void setpos(size_t y, size_t x) const;		// move cursor to pos
    int getmove(void) const;				// Return move
    void bell(void) const;				// Beep!
    void score(size_t i, const PLAYER& p);		// Post score
    void games(size_t i, const PLAYER& p);		// Post games
    void total(size_t i, const PLAYER& p);		// Post totals
    void ties(const PLAYER& p);				// Post ties
    __printflike(2, 3) __dead
    void abort(const char *s, ...) const;		// Algorithm error


  private:
    size_t	_ty, _tx;	// number of symbols in x and y dimension
    size_t	_ny, _nx;	// number of boxes in the x and y dimension
    int**	_b;		// board array of symbols
    GAMESCREEN*	_scrn;		// screen access, if we have one
};

#endif
