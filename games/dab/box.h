/*	$NetBSD: box.h,v 1.1.1.1.30.1 2008/05/18 12:29:48 yamt Exp $	*/

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
 * box.C: Single box utilities; A box is an entity with four edges, four
 *	  corners, and a center that maps directly to a board
 */

#ifndef _H_BOX
#define _H_BOX

#include <stdlib.h>

class BOARD;

class POINT {
  public:
    int x;
    int y;
};

class BOX {
  public:
    enum EDGE {
	first  = 0,
	top    = 0,
	bottom = 1,
	left   = 2,
	right  = 3,
	last   = 4,
    };

    BOX(size_t py, size_t px, BOARD& b);

    void reset(void);		// Clear a box
    void paint(void);		// Paint a box

    // Member access
    int& name(void);
    int& edge(int e);

    // Edge maniputations
    void set(int e);
    void clr(int e);
    int isset(int e) const;

    int count(void) const;	// Count the number of edges in use

    // Useful constants
    // Relative coordinates of the edges from the center of the box.
    static const POINT edges[BOX::last];
    // Relative coordinates of the corners from the center of the box.
    static const POINT corners[BOX::last];
    // Character symbols of the four edges
    static const int syms[BOX::last];

  private:
    void addcorner(size_t y, size_t x);	// add a corner character

    size_t _centerx;	// Coordinates of the center in board units
    size_t _centery;
    BOARD& _b;		// The board we refer to
};

#endif
