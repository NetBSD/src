/*	$NetBSD: iq80321_7seg.c,v 1.2.8.2 2002/04/12 20:50:28 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support for the 7-segment display on the Intel IQ80321.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/iq80321var.h>

#define	WRITE(x, v)	*((__volatile uint8_t *) (x)) = (v)

static int snakestate;

static const uint8_t digitmap[] = {
/*	+#####+
 *	#     #
 *	#     #
 *	#     #
 *	+-----+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 */
	SEG_G,

/*	+-----+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 */
	SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,

/*	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+#####+
 *	#     |
 *	#     |
 *	#     |
 *	+#####+
 */
	SEG_C|SEG_F,

/*	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+#####+
 */
	SEG_E|SEG_F,

/*	+-----+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 */
	SEG_A|SEG_D|SEG_E,

/*	+#####+
 *	#     |
 *	#     |
 *	#     |
 *	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+#####+
 */
	SEG_B|SEG_E,

/*	+#####+
 *	#     |
 *	#     |
 *	#     |
 *	+#####+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 */
	SEG_B,

/*	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 */
	SEG_D|SEG_E|SEG_F,

/*	+#####+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 */
	0,

/*	+#####+
 *	#     #
 *	#     #
 *	#     #
 *	+#####+
 *	|     #
 *	|     #
 *	|     #
 *	+-----+
 */
	SEG_D|SEG_E,
};

static uint8_t
iq80321_7seg_xlate(char c)
{
	uint8_t rv;

	if (c >= '0' && c <= '9')
		rv = digitmap[c - '0'];
	else if (c == '.')
		rv = ~SEG_DP;
	else
		rv = 0xff;

	return (rv);
}

void
iq80321_7seg(char a, char b)
{
	uint8_t msb, lsb;

	msb = iq80321_7seg_xlate(a);
	lsb = iq80321_7seg_xlate(b);

	snakestate = 0;

	WRITE(IQ80321_7SEG_MSB, msb);
	WRITE(IQ80321_7SEG_LSB, lsb);
}

static const uint8_t snakemap[][2] = {

/*	+#####+		+#####+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 */
	{ ~SEG_A & 0xff, ~SEG_A & 0xff },

/*	+-----+		+-----+
 *	#     |		|     #
 *	#     |		|     #
 *	#     |		|     #
 *	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 */
	{ ~SEG_F,	~SEG_B },

/*	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+#####+		+#####+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 */
	{ ~SEG_G,	~SEG_G },

/*	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 *	|     #		#     |
 *	|     #		#     |
 *	|     #		#     |
 *	+-----+		+-----+
 */
	{ ~SEG_C,	~SEG_E },

/*	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+#####+		+#####+
 */
	{ ~SEG_D,	~SEG_D },

/*	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 *	#     |		|     #
 *	#     |		|     #
 *	#     |		|     #
 *	+-----+		+-----+
 */
	{ ~SEG_E,	~SEG_C },

/*	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+#####+		+#####+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 */
	{ ~SEG_G,	~SEG_G },

/*	+-----+		+-----+
 *	|     #		#     |
 *	|     #		#     |
 *	|     #		#     |
 *	+-----+		+-----+
 *	|     |		|     |
 *	|     |		|     |
 *	|     |		|     |
 *	+-----+		+-----+
 */
	{ ~SEG_B,	~SEG_F },
};

void
iq80321_7seg_snake(void)
{
	int cur = snakestate;

	WRITE(IQ80321_7SEG_MSB, snakemap[cur][0]);
	WRITE(IQ80321_7SEG_LSB, snakemap[cur][1]);

	snakestate = (cur + 1) & 7;
}
