/* $NetBSD: dtmf.c,v 1.5 2026/04/03 08:37:46 mlelstv Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
 * Copyright (c) 2026 Michael van ELst <mlelstv@netbsd.org>
 * All rights reserved.
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

#include <sys/endian.h>

#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dtmf.h"

/* dialtone frequency pair */
struct dialtone {
	float a;
	float b;
};

#define	PI2	(3.14159265358979323846f * 2)

static void
dtmf_create(int16_t *buf, unsigned int sample_rate,
    unsigned short sample_length, unsigned short channels,
    unsigned int chanmask, float freq1, float freq2,
    unsigned attack, unsigned decay)
{
	int c;
	unsigned i;
	size_t sample_count = sample_rate * sample_length / 1000;
	float gain;
	int16_t v;

	for (i = 0; i < sample_count; i++) {
		if (i < attack)
			gain = 1.0 * i / attack;
		else if (sample_count - i < decay)
			gain = 1.0 * (sample_count - i) / decay;
		else
			gain = 1.0;
		for (c = 0; c < channels; c++) {
			if ((chanmask & (1 << c)) == 0)
				continue;
			v = (
				sin(i * PI2 * (freq1 / sample_rate)) +
				sin(i * PI2 * (freq2 / sample_rate))
			    ) * gain * 16383;
			buf[c] = htole16(v);
		}
		buf += channels;
	}
}

static void
dtmf_new(int16_t **buf, size_t *buflen, unsigned int sample_rate,
    unsigned short sample_length, unsigned short channels,
    unsigned int chanmask, struct dialtone dt)
{
	size_t oldlen, more;

	oldlen = *buflen;
	more = sample_rate * sample_length / 1000 * sizeof(**buf) * channels;

	*buflen = oldlen + more;
	*buf = realloc(*buf, *buflen);
	if (*buf == NULL) {
		warn("realloc");
		return;
	}
	memset((*buf) + oldlen/sizeof(**buf), 0, more);

	if (dt.a > 0 || dt.b > 0)
		dtmf_create((*buf) + oldlen/sizeof(**buf),
		    sample_rate, sample_length, channels, chanmask,
		    dt.a, dt.b, 500, 800);
}

static struct dialtone tones[] = {
	{ 697, 1209 }, /* 1 */
	{ 697, 1336 }, /* 2 */
	{ 697, 1477 }, /* 3 */
	{ 697, 1633 }, /* 4 */
	{ 770, 1209 }, /* 5 */
	{ 770, 1336 }, /* 6 */
	{ 770, 1477 }, /* 7 */
	{ 770, 1633 }, /* 8 */
	{ 852, 1209 }, /* 9 */
	{ 852, 1336 }, /* . */
	{ 852, 1477 }, /* 0 */
	{ 852, 1633 }, /* # */
	{ 941, 1209 }, /* A */
	{ 941, 1336 }, /* B */
	{ 941, 1477 }, /* C */
	{ 941, 1633 }  /* D */
};
#define TONES "123456789.0#ABCD"

static struct dialtone dialtone = { 350.0, 440.0 };
static struct dialtone reordertone = { 480.0, 620.0 };
static struct dialtone ringingtone = { 440.0, 480.0 };
static struct dialtone silence = { 0.0, 0.0 };
	
int
dtmf_dial(const char *number, unsigned int sample_rate,
	unsigned int channels, unsigned int chanmask,
	int16_t **bufp, size_t *buflenp)
{
	size_t buflen;
	int i;
	char ch, *p;

	*bufp = NULL;
	buflen = 0;

	/* dial tone */
	dtmf_new(bufp, &buflen, sample_rate, 2000,
	    channels, chanmask, dialtone);
	dtmf_new(bufp, &buflen, sample_rate, 65,
	    channels, chanmask, silence);

	if (*number == '\0')
		goto no_number;

	/* number */
	while ((ch = *number++) != '\0') {
		p = strchr(TONES, ch);
		if (p == NULL) {
			dtmf_new(bufp, &buflen, sample_rate, 130,
			    channels, chanmask, silence);
		} else {
			i = p - TONES;
			dtmf_new(bufp, &buflen, sample_rate, 65,
			    channels, chanmask, tones[i]);
			dtmf_new(bufp, &buflen, sample_rate, 65,
			    channels, chanmask, silence);
		}
	}

	/* pause */
	dtmf_new(bufp, &buflen, sample_rate, 325,
	    channels, chanmask, silence);

	if (chanmask & 0x55555555) {
		/* even channel */

		/* reorder tone */
		for (i=0; i<12; ++i) {
			dtmf_new(bufp, &buflen, sample_rate, 250,
			    channels, chanmask, reordertone);
			dtmf_new(bufp, &buflen, sample_rate, 250,
			    channels, chanmask, silence);
		}

		/* pause */
		dtmf_new(bufp, &buflen, sample_rate, 2000,
		    channels, chanmask, silence);
	} else {
		/* odd channel */

		/* ringing tone */
		dtmf_new(bufp, &buflen, sample_rate, 2000,
		    channels, chanmask, ringingtone);
		dtmf_new(bufp, &buflen, sample_rate, 4000,
		    channels, chanmask, silence);
		dtmf_new(bufp, &buflen, sample_rate, 2000,
		    channels, chanmask, ringingtone);

		/* pause */
		dtmf_new(bufp, &buflen, sample_rate, 250,
		    channels, chanmask, silence);
	}

no_number:
	*buflenp = buflen;
	return *bufp == NULL;
}
