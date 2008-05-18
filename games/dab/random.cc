/*	$NetBSD: random.cc,v 1.2.30.1 2008/05/18 12:29:48 yamt Exp $	*/

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
 * random.C: Randomizer for the dots program
 */

#include "defs.h"
RCSID("$NetBSD: random.cc,v 1.2.30.1 2008/05/18 12:29:48 yamt Exp $")

#include <time.h>
#include <string.h>
#include "random.h"

RANDOM::RANDOM(size_t ns) :
    _bs(ns)
{
    _bm = new char[(_bs >> 3) + 1];
    clear();
}

RANDOM::~RANDOM()
{
    delete[] _bm;
}

// Reinitialize
void RANDOM::clear(void)
{
    _nv = 0;
    ::srand48(::time(NULL));
    (void) ::memset(_bm, 0, (_bs >> 3) + 1);
}

// Return the next random value
size_t RANDOM::operator() (void)
{
    // No more values
    if (_nv == _bs)
	return _bs;

    for (;;) {
	size_t r = ::lrand48();
	size_t z = r % _bs;
        if (!isset(z)) {
	    set(z);
	    _nv++;
	    return z;
	}
    }
}
