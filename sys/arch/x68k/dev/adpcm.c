/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$NetBSD: adpcm.c,v 1.1.1.1.12.1 1997/10/14 10:20:05 thorpej Exp $
 */

#include "bsdaudio.h"
#if NBSDAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <x68k/dev/bsd_audiovar.h>
#include <x68k/dev/bsd_audioreg.h>
#include <x68k/dev/opmreg.h>
#include <machine/bsd_audioio.h>
#include <x68k/x68k/iodevice.h>

#define RATE_15K	15625
#define RATE_10K	10417
#define RATE_7K		 7813
#define RATE_5K		 5208
#define RATE_3K		 3906

struct adpcm_l2r {
	u_int	low;
	u_int	rate;
	u_char	clk;
	u_char	den;
} l2r[5] = {
	{ 13021, RATE_15K,	ADPCM_CLOCK_8MHZ, ADPCM_RATE_512},
	{  9115, RATE_10K,	ADPCM_CLOCK_8MHZ, ADPCM_RATE_768},
	{  6510, RATE_7K,	ADPCM_CLOCK_8MHZ, ADPCM_RATE_1024},
	{  4557, RATE_5K,	ADPCM_CLOCK_4MHZ, ADPCM_RATE_768},
	{     0, RATE_3K,	ADPCM_CLOCK_4MHZ, ADPCM_RATE_1024}
};

u_int
adpcm_round_sr(rate)
	u_int rate;
{
	int	i;
	for (i = 0; i < 5; i++) {
		if (rate >= l2r[i].low)
			return (l2r[i].rate);
	}
	/*NOTREACHED*/
}

void
adpcm_set_sr(rate)
	u_int rate;
{
	int	i;
	for (i = 0; i < 5; i++) {
		if (rate >= l2r[i].low) {
			PPI.portc = (PPI.portc & 0xf0) | l2r[i].den;
			adpcm_chgclk(l2r[i].clk);
			return;
		}
	}
	/*NOTREACHED*/
}

#endif
