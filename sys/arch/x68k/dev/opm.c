/*	$NetBSD: opm.c,v 1.3 1997/10/12 18:06:25 oki Exp $	*/

/*
 * Copyright (c) 1995 Masanobu Saitoh, Takuya Harakawa.
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
 *	This product includes software developed by Masanobu Saitoh.
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
 */

#include "fd.h"
/*#include "bsdaudio.h"*/
#include "bell.h"

#if ((NBSDAUDIO > 0) || (NFD > 0) || (NBELL > 0))

#include <sys/param.h>
#include <sys/systm.h>
#include <x68k/dev/opmreg.h>
#include <x68k/dev/bsd_audioreg.h>
#include <x68k/x68k/iodevice.h>

static u_char opmreg[0x100];
static struct opm_voice vdata[8];

void opm_set_volume __P((int, int));
void opm_set_key __P((int, int));
void opm_set_voice __P((int, struct opm_voice *));
void opm_set_voice_sub __P((int, struct opm_operator *));
__inline static void writeopm __P((int, int));
__inline static int readopm __P((int));
void opm_key_on __P((u_char));
void opm_key_off __P((u_char));
int opmopen __P((dev_t, int, int));
int opmclose __P((dev_t));

__inline static void
writeopm(reg, dat)
	int reg, dat;
{
	while (OPM.data & 0x80);
	OPM.reg = reg;
	while (OPM.data & 0x80);
	OPM.data = opmreg[reg] = dat;
}

__inline static int
readopm(reg)
	int reg;
{
	return opmreg[reg];
}

void
adpcm_chgclk(clk)
	u_char	clk;
{
	writeopm(0x1b, readopm(0x1b) & ~OPM1B_CT1MSK | clk);
}

void
fdc_force_ready(rdy)
	u_char	rdy;
{
	writeopm(0x1b, readopm(0x1b) & ~OPM1B_CT2MSK | rdy);
}

void
opm_key_on(channel)
	u_char channel;
{
    writeopm(0x08, vdata[channel].sm << 3 | channel);
}

void
opm_key_off(channel)
	u_char	channel;
{
    writeopm(0x08, channel);
}

void
opm_set_voice(channel, voice)
	int channel;
	struct opm_voice *voice;
{
	bcopy(voice, &vdata[channel], sizeof(struct opm_voice));

	opm_set_voice_sub(0x40 + channel, &voice->m1);
	opm_set_voice_sub(0x48 + channel, &voice->m2);
	opm_set_voice_sub(0x50 + channel, &voice->c1);
	opm_set_voice_sub(0x58 + channel, &voice->c2);
	writeopm(0x20 + channel, 0xc0 | (voice->fb & 0x7) << 3 | (voice->con & 0x7));
}

void
opm_set_voice_sub(reg, op)
	register int reg;
	struct opm_operator *op;
{
    /* DT1/MUL */
    writeopm(reg, (op->dt1 & 0x7) << 3 | (op->mul & 0x7));

    /* TL */
    writeopm(reg + 0x20, op->tl & 0x7f);

    /* KS/AR */
    writeopm(reg + 0x40, (op->ks & 0x3) << 6 | (op->ar & 0x1f));

    /* AMS/D1R */
    writeopm(reg + 0x60, (op->ame & 0x1) << 7 | (op->d1r & 0x1f));

    /* DT2/D2R */
    writeopm(reg + 0x80, (op->dt2 & 0x3) << 6 | (op->d2r & 0x1f));

    /* D1L/RR */
    writeopm(reg + 0xa0, (op->d1l & 0xf) << 4 | (op->rr & 0xf));
}

void
opm_set_volume(channel, volume)
	int channel;
	int volume;
{
    int value;

    switch (vdata[channel].con) {
    case 7:
	value = vdata[channel].m1.tl + volume;
	writeopm(0x60 + channel, ((value > 0x7f) ? 0x7f : value));
    case 6:
    case 5:
	value = vdata[channel].m2.tl + volume;
	writeopm(0x68 + channel, ((value > 0x7f) ? 0x7f : value));
    case 4:
	value = vdata[channel].c1.tl + volume;
	writeopm(0x70 + channel, ((value > 0x7f) ? 0x7f : value));
    case 3:
    case 2:
    case 1:
    case 0:
	value = vdata[channel].c2.tl + volume;
	writeopm(0x78 + channel, ((value > 0x7f) ? 0x7f : value));
    }
}

void
opm_set_key(channel, tone)
	int channel;
	int tone;
{
	writeopm(0x28 + channel, tone >> 8);
	writeopm(0x30 + channel, tone & 0xff);
}

/*ARGSUSED*/
int
opmopen(dev, flag, mode)
	dev_t dev;
	int flag, mode;
{
	return 0;
}

/*ARGSUSED*/
int
opmclose(dev)
	dev_t dev;
{
	return 0;
}

#endif
