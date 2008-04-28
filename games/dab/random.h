/*	$NetBSD: random.h,v 1.3 2008/04/28 20:22:54 martin Exp $	*/

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
 * random.h: Randomizer; returns a random sequence of values from [0..fx) 
 *	     returning each value exactly once. After fx calls it returns fx.
 */

#ifndef _H_RANDOM
#define _H_RANDOM

#include <stdlib.h>

class RANDOM {
  public:
    // Constructor and destructor
    RANDOM(size_t fx);
    ~RANDOM();

    size_t operator () (void);	// Next random
    void clear(void);		// Reset

  private:

    int isset(size_t z) {
	return (_bm[z >> 3] & (1 << (z & 7))) != 0;
    }

    void set(size_t z) {
	_bm[z >> 3] |= (1 << (z & 7));
    }

    char*	_bm;	// Bitmap indicating the numbers used
    size_t	_nv;	// Number of values returned so far
    size_t	_bs;	// Maximum value
};

#endif
