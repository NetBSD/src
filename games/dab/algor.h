/*	$NetBSD: algor.h,v 1.3 2006/05/14 03:21:52 christos Exp $	*/

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
 * algor.h: Computer's algorithm
 */

#ifndef _H_ALGOR
#define _H_ALGOR

#include "player.h"

class BOARD;
class BOX;

class ALGOR : public PLAYER {
  public:
    ALGOR(const char c);
    virtual ~ALGOR() {}
    // Return a proposed move in (y, x, dir)
    void play(const BOARD& b, size_t& y, size_t& x, int& dir);

  private:
    // Closure searches
    int find_closure(size_t& y, size_t& x, int& dir, BOARD& b);
    size_t find_max_closure(size_t& y, size_t& x, int& dir, const BOARD& b);
    size_t find_min_closure1(size_t& y, size_t& x, int& dir, const BOARD& b,
	int last);
    size_t find_min_closure(size_t& y, size_t& x, int& dir, const BOARD& b);

    // Move searches
    int find_good_turn(size_t& y, size_t& x, int& dir, const BOARD& b);
    int find_bad_turn(size_t& y, size_t& x, int& dir, BOARD& b, int last);

    // Move Attempts
    int try_bad_turn(BOX& box, size_t& y, size_t& x, int& dir, BOARD& b,
		     int last);
    int try_good_turn(BOX& box, size_t y, size_t x, int& dir, BOARD& b);

    // Utils
    size_t count_closure(size_t& y, size_t& x, int& dir, BOARD& b);

#ifdef notyet
    size_t find_single(void);
#endif

    size_t _edge1;
    size_t _edge2;
    size_t _maxedge;
};

#endif
