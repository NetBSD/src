/* $NetBSD: splash.c,v 1.6.12.1 2009/05/13 17:21:29 jym Exp $ */

/*-
 * Copyright (c) 2006 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: splash.c,v 1.6.12.1 2009/05/13 17:21:29 jym Exp $");

#include "opt_splash.h"

/* XXX */
#define	NSPLASH8 1
#define	NSPLASH16 1
#define	NSPLASH32 1

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#include <dev/splash/splash.h>

#if !defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
#error "options SPLASHSCREEN_PROGRESS requires SPLASHSCREEN"
#endif

#ifdef __HAVE_CPU_COUNTER
#include <sys/cpu.h>
#include <machine/cpu_counter.h>
#endif

#ifndef SPLASHSCREEN_IMAGE
#define SPLASHSCREEN_IMAGE "dev/splash/images/netbsd.h"
#endif

#ifdef SPLASHSCREEN
#include SPLASHSCREEN_IMAGE

#ifdef SPLASHSCREEN_PROGRESS
struct splash_progress *splash_progress_state;
#ifdef __HAVE_CPU_COUNTER
static uint64_t splash_last_update;
#endif
#endif

#if NSPLASH8 > 0
static void	splash_render8(struct splash_info *, const char *, int,
			       int, int, int, int);
#endif
#if NSPLASH16 > 0
static void	splash_render16(struct splash_info *, const char *, int,
				int, int, int, int);
#endif
#if NSPLASH32 > 0
static void	splash_render32(struct splash_info *, const char *, int,
				int, int, int, int);
#endif

void
splash_render(struct splash_info *si, int flg)
{
	int xoff, yoff;

	/* XXX */
	if (flg & SPLASH_F_CENTER) {
		xoff = (si->si_width - _splash_width) / 2;
		yoff = (si->si_height - _splash_height) / 2;
	} else
		xoff = yoff = 0;

	switch (si->si_depth) {
#if NSPLASH8 > 0
	case 8:
		splash_render8(si, _splash_header_data, xoff, yoff,
		    _splash_width, _splash_height, flg);
		break;
#endif
#if NSPLASH16 > 0
	case 16:
		splash_render16(si, _splash_header_data, xoff, yoff,
		    _splash_width, _splash_height, flg);
		break;
#endif
#if NSPLASH32 > 0
	case 32:
		splash_render32(si, _splash_header_data, xoff, yoff,
		    _splash_width, _splash_height, flg);
		break;
#endif
	default:
		aprint_error("WARNING: Splash not supported at %dbpp\n",
		    si->si_depth);
		break;
	}

	return;
}

#if NSPLASH8 > 0
static void
splash_render8(struct splash_info *si, const char *data, int xoff, int yoff,
	       int swidth, int sheight, int flg)
{
	const char *p;
	u_char *fb, pix;
	int x, y, i;
	int filled;

	fb = si->si_bits;
	if (flg & SPLASH_F_FILL)
		filled = 0;
	else
		filled = 1;

	p = data;
	fb += xoff + (yoff * si->si_width);
	for (y = 0; y < sheight; y++) {
		for (x = 0; x < swidth; x++) {
			pix = *p++;
			pix += SPLASH_CMAP_OFFSET;
			if (filled == 0) {
				for (i = 0; i < (si->si_width * si->si_height);
				     i++)
					si->si_bits[i] = pix;
				filled = 1;
			}
			fb[x] = pix;
		}
		fb += si->si_width;
	}

	/* If we've just written to the shadow fb, copy it to the display */
	if (si->si_hwbits) {
		if (flg & SPLASH_F_FILL) {
			memcpy(si->si_hwbits, si->si_bits,
			    si->si_width*si->si_height);
		} else {
			u_char *rp, *hrp;

			rp = si->si_bits + xoff + (yoff * si->si_width);
			hrp = si->si_hwbits + xoff + (yoff * si->si_width);

			for (y = 0; y < sheight; y++) {
				memcpy(hrp, rp, swidth);
				hrp += si->si_stride;
				rp += si->si_stride;
			}
		}
	}

	return;
}
#endif /* !NSPLASH8 > 0 */

#if NSPLASH16 > 0
#define RGBTO16(b, o, x, c)					\
	do {							\
		uint16_t *_ptr = (uint16_t *)(&(b)[(o)]);	\
		*_ptr = (((c)[(x)*3+0] / 8) << 11) |		\
			(((c)[(x)*3+1] / 4) << 5) |		\
			(((c)[(x)*3+2] / 8) << 0);		\
	} while (0)

static void
splash_render16(struct splash_info *si, const char *data, int xoff, int yoff,
		int swidth, int sheight, int flg)
{
	const char *d;
	u_char *fb, *p;
	u_char pix[3];
	int x, y, i;
	int filled;

	fb = si->si_bits;

	if (flg & SPLASH_F_FILL)
		filled = 0;
	else
		filled = 1;

	d = data;
	fb += xoff * 2 + yoff * si->si_stride;

	for (y = 0; y < sheight; y++) {
		for (x = 0; x < swidth; x++) {
			_SPLASH_HEADER_PIXEL(d, pix);
			if (filled == 0) {
				p = si->si_bits;
				i = 0;
				while (i < si->si_height*si->si_stride) {
					RGBTO16(p, i, 0, pix);
					i += 2;
				}
				filled = 1;
			}
			RGBTO16(fb, x*2, 0, pix);
		}
		fb += si->si_stride;
	}

	/* If we've just written to the shadow fb, copy it to the display */
	if (si->si_hwbits) {
		if (flg & SPLASH_F_FILL) {
			memcpy(si->si_hwbits, si->si_bits,
			    si->si_height*si->si_stride);
		} else {
			u_char *rp, *hrp;

			rp = si->si_bits + (xoff * 2) + (yoff * si->si_stride);
			hrp = si->si_hwbits + (xoff * 2) +
			    (yoff * si->si_stride);

			for (y = 0; y < sheight; y++) {
				memcpy(hrp, rp, swidth * 2);
				rp += si->si_stride;
				hrp += si->si_stride;
			}
		}
	}

	return;
}
#undef RGBTO16
#endif /* !NSPLASH16 > 0 */

#if NSPLASH32 > 0
static void
splash_render32(struct splash_info *si, const char *data, int xoff, int yoff,
		int swidth, int sheight, int flg)
{
	const char *d;
	u_char *fb, *p;
	u_char pix[3];
	int x, y, i;
	int filled;

	fb = si->si_bits;

	if (flg & SPLASH_F_FILL)
		filled = 0;
	else
		filled = 1;

	d = data;
	fb += xoff * 4 + yoff * si->si_stride;

	for (y = 0; y < sheight; y++) {
		for (x = 0; x < swidth; x++) {
			_SPLASH_HEADER_PIXEL(d, pix);
			if (filled == 0) {
				p = si->si_bits;
				i = 0;
				while (i < si->si_height*si->si_stride) {
					p[i++] = pix[2];
					p[i++] = pix[1];
					p[i++] = pix[0];
					p[i++] = 0;
				}
				filled = 1;
			}
			fb[x*4+0] = pix[2];
			fb[x*4+1] = pix[1];
			fb[x*4+2] = pix[0];
			fb[x*4+3] = 0;
		}
		fb += si->si_stride;
	}

	/* If we've just written to the shadow fb, copy it to the display */
	if (si->si_hwbits) {
		if (flg & SPLASH_F_FILL) {
			memcpy(si->si_hwbits, si->si_bits,
			    si->si_height*si->si_stride);
		} else {
			u_char *rp, *hrp;

			rp = si->si_bits + (xoff * 4) + (yoff * si->si_stride);
			hrp = si->si_hwbits + (xoff * 4) +
			    (yoff * si->si_stride);

			for (y = 0; y < sheight; y++) {
				memcpy(hrp, rp, swidth * 4);
				rp += si->si_stride;
				hrp += si->si_stride;
			}
		}
	}

	return;
}
#endif /* !NSPLASH32 > 0 */

#ifdef SPLASHSCREEN_PROGRESS

static void
splash_progress_render(struct splash_progress *sp)
{
	struct splash_info *si;
	int i;
	int w;
	int spacing;
	int xoff;
	int yoff;
	int flg;

	si = sp->sp_si;
	flg = 0;

	/* where should we draw the pulsers? */
	yoff = (si->si_height / 8) * 7;
	w = _pulse_off_width * SPLASH_PROGRESS_NSTATES;
	xoff = (si->si_width / 4) * 3;
	spacing = _pulse_off_width; /* XXX */

	for (i = 0; i < SPLASH_PROGRESS_NSTATES; i++) {
		const char *d = (sp->sp_state == i ? _pulse_on_header_data :
				 _pulse_off_header_data);
		switch (si->si_depth) {
#if NSPLASH8 > 0
		case 8:
			splash_render8(si, d, (xoff + (i * spacing)),
			    yoff, _pulse_off_width, _pulse_off_height, flg);
			break;
#endif
#if NSPLASH16 > 0
		case 16:
			splash_render16(si, d, (xoff + (i * spacing)),
			    yoff, _pulse_off_width, _pulse_off_height, flg);
			break;
#endif
#if NSPLASH32 > 0
		case 32:
			splash_render32(si, d, (xoff + (i * spacing)),
			    yoff, _pulse_off_width, _pulse_off_height, flg);
			break;
#endif
		default:
			/* do nothing */
			break;
		}
	}
}

static int
splash_progress_stop(device_t dev)
{
	struct splash_progress *sp;

	sp = (struct splash_progress *)dev;
	sp->sp_running = 0;

	return 0;
}

void
splash_progress_init(struct splash_progress *sp)
{
#ifdef __HAVE_CPU_COUNTER
	if (cpu_hascounter())
		splash_last_update = cpu_counter();
	else
		splash_last_update = 0;
#endif

	sp->sp_running = 1;
	sp->sp_force = 0;
	splash_progress_state = sp;
	splash_progress_render(sp);
	config_finalize_register((device_t)sp, splash_progress_stop);

	return;
}

void
splash_progress_update(struct splash_progress *sp)
{
	if (sp->sp_running == 0 && sp->sp_force == 0)
		return;

#ifdef __HAVE_CPU_COUNTER
	if (cpu_hascounter()) {
		uint64_t now;

		if (splash_last_update == 0) {
			splash_last_update = cpu_counter();
		} else {
			now = cpu_counter();
			if (splash_last_update + cpu_frequency(curcpu())/4 >
			    now)
				return;
			splash_last_update = now;
		}
	}
#endif
	sp->sp_state++;
	if (sp->sp_state >= SPLASH_PROGRESS_NSTATES)
		sp->sp_state = 0;

	splash_progress_render(sp);
}

#endif /* !SPLASHSCREEN_PROGRESS */

#endif /* !SPLASHSCREEN */
