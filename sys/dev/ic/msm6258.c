/*	$NetBSD: msm6258.c,v 1.9 2002/04/07 14:51:40 isaki Exp $	*/

/*
 * Copyright (c) 2001 Tetsuya Isaki. All rights reserved.
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
 *      This product includes software developed by Tetsuya Isaki.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * OKI MSM6258 ADPCM voice synthesizer codec.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msm6258.c,v 1.9 2002/04/07 14:51:40 isaki Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/audiovar.h>
#include <dev/mulaw.h>
#include <dev/ic/msm6258var.h>

struct msm6258_codecvar {
	/* ADPCM stream must be converted in order. */
	u_char	mc_buf[AU_RING_SIZE]; /* XXX */

	short	mc_amp;
	char	mc_estim;
};

struct msm6258_softc {
	struct device sc_dev;
	struct msm6258_codecvar *sc_mc;
	/* MD vars follow */
};

static inline u_char pcm2adpcm_step(struct msm6258_codecvar *, short);
static inline short  adpcm2pcm_step(struct msm6258_codecvar *, u_char);

static int adpcm_estimindex[16] = {
	 2,  6,  10,  14,  18,  22,  26,  30,
	-2, -6, -10, -14, -18, -22, -26, -30
};

static int adpcm_estim[49] = {
	 16,  17,  19,  21,  23,  25,  28,  31,  34,  37,
	 41,  45,  50,  55,  60,  66,  73,  80,  88,  97,
	107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
	279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 875, 963, 1060, 1166, 1282, 1411, 1552
};

static int adpcm_estimstep[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

void *
msm6258_codec_init (void)
{
	struct msm6258_codecvar *r;

	r = malloc (sizeof(*r), M_DEVBUF, M_NOWAIT);
	if (r == 0)
		return 0;
	r->mc_amp = r->mc_estim = 0;

	return r;
}

int
msm6258_codec_open(void *hdl)
{
	struct msm6258_softc *sc = hdl;
	struct msm6258_codecvar *mc = sc->sc_mc;

	mc->mc_amp = 0;
	mc->mc_estim = 0;

	return 0;
}

/*
 * signed 16bit linear PCM -> OkiADPCM
 */
static inline u_char
pcm2adpcm_step(struct msm6258_codecvar *mc, short a)
{
	int estim = (int)mc->mc_estim;
	int df;
	short dl, c;
	unsigned char b;
	unsigned char s;

	df = a - mc->mc_amp;
	dl = adpcm_estim[estim];
	c = (df / 16) * 8 / dl;
	if (df < 0) {
		b = (unsigned char)(-c) / 2;
		s = 0x08;
	} else {
		b = (unsigned char)(c) / 2;
		s = 0;
	}
	if (b > 7)
		b = 7;
	s |= b;
	mc->mc_amp += (short)(adpcm_estimindex[(int)s] * dl);
	estim += adpcm_estimstep[b];
	if (estim < 0)
		estim = 0;
	else if (estim > 48)
		estim = 48;

	mc->mc_estim = estim;
	return s;
}

void
msm6258_slinear16_host_to_adpcm(void *hdl, u_char *p, int cc)
{
	struct msm6258_softc *sc = hdl;
	struct msm6258_codecvar *mc = sc->sc_mc;
	short *s = (short *)p;
	int i;
	u_char f;

	for (i = 0; i < cc; i += 4) {
		f  = pcm2adpcm_step(mc, *s++);
		f |= pcm2adpcm_step(mc, *s++) << 4;
		*p++ = f;
	}
}

void
msm6258_slinear16_le_to_adpcm(void *hdl, u_char *p, int cc)
{
#if BYTE_ORDER == BIG_ENDIAN
	swap_bytes(hdl, p, cc);
#endif
	msm6258_slinear16_host_to_adpcm(hdl, p, cc);
}

void
msm6258_slinear16_be_to_adpcm(void *hdl, u_char *p, int cc)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	swap_bytes(hdl, p, cc);
#endif
	msm6258_slinear16_host_to_adpcm(hdl, p, cc);
}

void
msm6258_slinear8_to_adpcm(void *hdl, u_char *p, int cc)
{
	struct msm6258_softc *sc = hdl;
	struct msm6258_codecvar *mc = sc->sc_mc;
	u_char *s = p;
	int i;
	u_char f;

	for (i = 0; i < cc; i += 2) {
		f  = pcm2adpcm_step(mc, (short)(*s++) * 256);
		f |= pcm2adpcm_step(mc, (short)(*s++) * 256) << 4;
		*p++ = f;
	}
}

void
msm6258_ulinear8_to_adpcm(void *hdl, u_char *p, int cc)
{
	change_sign8(hdl, p, cc);
	msm6258_slinear8_to_adpcm(hdl, p, cc);
}

void
msm6258_mulaw_to_adpcm(void *hdl, u_char *p, int cc)
{
	mulaw_to_slinear8(hdl, p, cc);
	msm6258_slinear8_to_adpcm(hdl, p, cc);
}


/*
 * OkiADPCM -> signed 16bit linear PCM
 */
static inline short
adpcm2pcm_step(struct msm6258_codecvar *mc, u_char b)
{
	int estim = (int)mc->mc_estim;

	mc->mc_amp += adpcm_estim[estim] * adpcm_estimindex[b];
	estim += adpcm_estimstep[b];

	if (estim < 0)
		estim = 0;
	else if (estim > 48)
		estim = 48;

	mc->mc_estim = estim;

	return mc->mc_amp;
}

void
msm6258_adpcm_to_slinear16_host(void *hdl, u_char *p, int cc)
{
	struct msm6258_softc *sc = hdl;
	struct msm6258_codecvar *mc = sc->sc_mc;
	short *d = (short *)p;
	int i;
	u_char a;

	/* XXX alignment ? */
	memcpy(mc->mc_buf, p, cc / 4);
	for (i = 0; i < cc / 4;) {
		a = mc->mc_buf[i++];

		*d++ = adpcm2pcm_step(mc, a & 0x0f);
		*d++ = adpcm2pcm_step(mc, (a >> 4) & 0x0f);
	}
}

void
msm6258_adpcm_to_slinear16_le(void *hdl, u_char *p, int cc)
{
	msm6258_adpcm_to_slinear16_host(hdl, p, cc);
#if BYTE_ORDER == BIG_ENDIAN
	swap_bytes(hdl, p, cc);
#endif
}

void
msm6258_adpcm_to_slinear16_be(void *hdl, u_char *p, int cc)
{
	msm6258_adpcm_to_slinear16_host(hdl, p, cc);
#if BYTE_ORDER == LITTLE_ENDIAN
	swap_bytes(hdl, p, cc);
#endif
}

void
msm6258_adpcm_to_slinear8(void *hdl, u_char *p, int cc)
{
	struct msm6258_softc *sc = hdl;
	struct msm6258_codecvar *mc = sc->sc_mc;
	char *d = (char *)p;
	int i;
	u_char a;

	/* cc may be even. XXX alignment ? */
	memcpy(mc->mc_buf, p, cc / 2);
	for (i = 0; i < cc / 2;) {
		a = mc->mc_buf[i++];

		*d++ = adpcm2pcm_step(mc, a & 0x0f) / 256;
		*d++ = adpcm2pcm_step(mc, (a >> 4) & 0x0f) / 256;
	}
}

void
msm6258_adpcm_to_ulinear8(void *hdl, u_char *p, int cc)
{
	msm6258_adpcm_to_slinear8(hdl, p, cc);
	change_sign8(hdl, p, cc);
}

void
msm6258_adpcm_to_mulaw(void *hdl, u_char *p, int cc)
{
	msm6258_adpcm_to_slinear8(hdl, p, cc);
	slinear8_to_mulaw(hdl, p, cc);
}

