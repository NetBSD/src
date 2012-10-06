/*	$NetBSD: ttyscrn.h,v 1.4 2012/10/06 19:39:51 christos Exp $	*/

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
 * ttyscrn.h: Curses based screen for dots
 */

#ifndef _H_TTYSCRN
#define _H_TTYSCRN

#include "gamescreen.h"

class TTYSCRN : public GAMESCREEN {
  public:
    // Constructor that can fail
    static TTYSCRN*  create(int acs, size_t *y, size_t *x);
    ~TTYSCRN();

    // Screen virtuals
    void clean(void);
    void moveto(size_t y, size_t x);
    void addsym(const int sym);
    void addedge(const int sym);
    void redraw(void);
    void bell(void);
    int getinput(void);
    void score(size_t s, const PLAYER& p);
    void games(size_t s, const PLAYER& p);
    void total(size_t s, const PLAYER& p);
    void ties(const PLAYER& p);

  private:
    enum {
	offsx = 2,	// board x offset from top left corner
	offsy = 2,	// board y offset from top left corner
	offsscore = 0,	// score y offset from top of the board
	offstotal = 3,	// total y offset from top of the board
	offsgames = 6,	// games y offset from top of the board
	offsties = 8	// ties y offset from top of the board
    };
    size_t _sx, _sy;	// board size
    size_t _tx, _ty;	// tty size
    int _acs;		// do we want acs?
};

#endif
