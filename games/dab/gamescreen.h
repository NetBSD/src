/*	$NetBSD: gamescreen.h,v 1.1.1.1 2003/12/26 17:57:03 christos Exp $	*/

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
 * screen.h: Screen base class
 */

#ifndef _H_GAMESCREEN
#define _H_GAMESCREEN

#include <stdlib.h>

class PLAYER;

class GAMESCREEN {
  public:
    enum EDGE {
	GS_HLINE,
	GS_VLINE,
	GS_ULCORNER,
	GS_URCORNER,
	GS_LLCORNER,
	GS_LRCORNER,
	GS_LTEE,
	GS_RTEE,
	GS_TTEE,
	GS_BTEE,
	GS_PLUS
    };
    virtual ~GAMESCREEN();
    virtual void clean(void) = 0;			// Clear screen
    virtual void moveto(size_t y, size_t x) = 0;	// Move to x, y
    virtual void addsym(const int sym) = 0;		// Add character symbol
    virtual void addedge(const int sym) = 0;		// Add character symbol
    virtual void redraw(void) = 0;			// Refresh
    virtual int getinput(void) = 0;			// Get user input
    virtual void bell(void) = 0;			// Beep
    virtual void score(size_t p, const PLAYER& p) = 0;	// Post current score
    virtual void games(size_t p, const PLAYER& p) = 0;	// Post games won
    virtual void total(size_t p, const PLAYER& p) = 0;	// Post total score
    virtual void ties(const PLAYER& p) = 0;		// Post tie games
};

#endif
