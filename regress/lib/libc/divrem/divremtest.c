/*	$NetBSD: divremtest.c,v 1.4 2002/02/21 07:38:15 itojun Exp $	*/

/*-
 * Copyright (c) 2002 Ross Harvey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    Redistributions of source code must not add the GNU General Public
 *    License or any similar license to this work or to derivative works
 *    incorporating this code. No requirement to distribute source may
 *    be imposed upon redistributions in binary form of this work or of
 *    derivative works.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#define	KLE	3	// exponent for k and l salt values
#define	NEARBY	600	// all numbers +-NEARBY 0 and INTxx_MIN will be tried
#define	RANDOMCOUNT 300000	// number of random(3)-based cases to run

#define	 IM(x)	 ((intmax_t)(x))
#define	UIM(x)	((uintmax_t)(x))

#define TEST(id, a, b, c)                             			\
    if (b) {                            				\
	c = (a) / (b);                        				\
	printf(id "%16jx / %16jx => %16jx\n", UIM(a), UIM(b), UIM(c));  \
	c = (a) % (b);                        				\
	printf(id "%16jx / %16jx => %16jx\n", UIM(a), UIM(b), UIM(c));  \
    }
    
#define	T64S(a, b, c)	TEST("64 ", (a), (b), (c))
#define	T64U(a, b, c)	TEST("64U", (a), (b), (c))

union {
	char	ranstate[128];
	long	alignme;
} ranalign;

int enable_time_output;

void mark_time(const int phase)
{
static	time_t startphase;
	time_t t;

	t = time(NULL);
	if (enable_time_output && phase != 0) {
		fprintf(stderr, "phase %d/6: %5d seconds\n", phase,
		    (int)(t - startphase));
		fflush(stderr);
	}
	startphase = t;
}

int main(int ac, char **av)
{
     int32_t a32, b32, sr32;
    uint32_t ur32;
     intmax_t a64, b64, sr64;
    uintmax_t ur64;
     int i, j, k, l;

    enable_time_output = ac <= 1;
    mark_time(0);
    for(i = KLE; i <= 30; ++i) {
	a32 = 1 << i;
	for(j = KLE; j <= 30; ++j) {
	    b32 = 1 << j;
	    for(k = -(1 << KLE); k <= 1 << KLE; ++k) {
		for(l = -(1 << KLE); l <= 1 << KLE; ++l) {
		    TEST("32 ",  a32 + k,  b32 + l, sr32);
		    TEST("32 ",  a32 + k, -(b32 + l), sr32);
		    TEST("32 ", -(a32 + k),   b32 + l, sr32);
		    TEST("32 ", -(a32 + k), -(b32 + l), sr32);
		    assert((1U << i) + k >= 0);
		    assert((1U << j) + l >= 0);
		    TEST("32U", (1U << i) + k, (1U << j) + l, ur32);
		}
	    }
	}
    }

    mark_time(1);
    for(a32 = -NEARBY; a32 < NEARBY; ++a32) {
	for(b32 = -NEARBY; b32 < NEARBY; ++b32) {
	    TEST("32 ", a32, b32, sr32);
	    if (a32 >= 0 && b32 >= 0)
		TEST("32U", (unsigned)a32, (unsigned)b32, ur32);
	}
    }
    mark_time(2);
    for(a32 = INT32_MIN; a32 < INT32_MIN + NEARBY; ++a32) {
	for(b32 = INT32_MIN; b32 < INT32_MIN + NEARBY; ++b32)
	    TEST("32 ", a32, b32, sr32);
	for(b32 = -NEARBY; b32 < NEARBY; ++b32)
	    if (a32 != INT32_MIN || b32 != -1)
		TEST("32 ", a32, b32, sr32);
    }

    mark_time(3);
    if (sizeof(intmax_t) == 4)
	exit(0);
    for(i = KLE; i <= 62; ++i) {
	a64 = IM(1) << i;
	for(j = KLE; j <= 62; ++j) {
	    b64 = IM(1) << j;
	    for(k = -(1 << KLE); k <= 1 << KLE; ++k) {
		for(l = -(1 << KLE); l <= 1 << KLE; ++l) {
		    T64S( a64 + k,  b64 + l, sr64);
		    T64S( a64 + k, -b64 + l, sr64);
		    T64S(-a64 + k,  b64 + l, sr64);
		    T64S(-a64 + k, -b64 + l, sr64);
		    T64U(UIM(a64) + k, UIM(b64) + l, ur64);
		}
	    }
	}
    }

    mark_time(4);
    for(a64 = -(1 << KLE); a64 < 1 << KLE; ++a64) {
	for(b64 = -(1 << KLE); b64 < 1 << KLE; ++b64) {
	    TEST("64 ", a64, b64, sr64);
	    if (a64 >= 0 && b64 >= 0)
		TEST("64U", (unsigned)a64, (unsigned)b64, ur64);
	}
    }
    for(a64 = INT64_MIN; a64 < INT64_MIN + NEARBY; ++a64) {
	for(b64 = INT64_MIN; b64 < INT64_MIN + NEARBY; ++b64)
	    TEST("64 ", a64, b64, sr64);
	for(b64 = -NEARBY; b64 < NEARBY; ++b64)
	    if (a64 != INT64_MIN || b64 != -1)
		TEST("64 ", a64, b64, sr64);
    }
    mark_time(5);
    initstate(1UL, ranalign.ranstate, sizeof ranalign.ranstate);
    for(i = 0; i < RANDOMCOUNT; ++i) {
	int32_t low32 = random();
	int64_t low64 = (intmax_t)random() << 32 | low32;

	a32 = random();
	b32 = random();
	a64 = ((intmax_t)random() << 32) | a32;
	b64 = ((intmax_t)random() << 32) | b32;
	TEST("32 ", a32, b32, sr32);
	TEST("32u", (unsigned)a32 + low32, (unsigned)b32 + low32, ur32);
	TEST("32 ", -a32 - 1, b32, sr32);
	TEST("32 ", a32, -b32, sr32);
	if (a32 != INT32_MAX || b32 != 1)
	    TEST("32 ", -a32 - 1, -b32, sr32);
	TEST("64 ", a64, b64, sr64);
	TEST("64u", (unsigned)a64 + low64, (unsigned)b64 + low64, ur64);
	TEST("64 ", -a64 - 1, b64, sr64);
	TEST("64 ", a64, -b64, sr64);
	if (a64 != INT64_MAX || b64 != 1)
	    TEST("64 ", -a64 - 1, -b64, sr64);
    }
    mark_time(6);
    exit(0);
    return 0;
}
