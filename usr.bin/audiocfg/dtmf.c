/* $NetBSD: dtmf.c,v 1.1 2010/09/01 09:04:16 jmcneill Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dtmf.h"

#define	PI2	(3.14159265358979323846f * 2)

static void
dtmf_create(int16_t *buf, unsigned int sample_rate,
    unsigned short sample_length, unsigned short channels,
    unsigned int chanmask, float freq1, float freq2)
{
	int c, i;
	size_t sample_count = sample_rate * sample_length;

	for (i = 0; i < sample_count; i++) {
		for (c = 0; c < channels; c++) {
			if ((chanmask & (1 << c)) == 0)
				continue;
			buf[c] = 
			    (sin(i * PI2 * (freq1 / sample_rate)) +
			     sin(i * PI2 * (freq2 / sample_rate))) * 16383;
		}
		buf += channels;
	}
}

void
dtmf_new(int16_t **buf, size_t *buflen, unsigned int sample_rate,
    unsigned short sample_length, unsigned short channels,
    unsigned int chanmask, float rate1, float rate2)
{
	*buflen = sample_rate * sizeof(int16_t) * sample_length * channels;
	*buf = calloc(1, *buflen);
	if (*buf == NULL) {
		perror("calloc");
		return;
	}

	dtmf_create(*buf, sample_rate, sample_length, channels, chanmask,
	    rate1, rate2);
}
