/* 	$NetBSD: rasops1.c,v 1.29 2019/07/29 02:57:41 rin Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops1.c,v 1.29 2019/07/29 02:57:41 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <machine/endian.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>
#include <dev/rasops/rasops_masks.h>

static void	rasops1_copycols(void *, int, int, int, int);
static void	rasops1_erasecols(void *, int, int, int, long);
static void	rasops1_do_cursor(struct rasops_info *);
static void	rasops1_putchar(void *, int, int col, u_int, long);
#ifndef RASOPS_SMALL
static void	rasops1_putchar8(void *, int, int col, u_int, long);
static void	rasops1_putchar16(void *, int, int col, u_int, long);
#endif

/*
 * Initialize rasops_info struct for this colordepth.
 */
void
rasops1_init(struct rasops_info *ri)
{

	if ((ri->ri_font->fontwidth & 7) != 0) {
		ri->ri_ops.erasecols = rasops1_erasecols;
		ri->ri_ops.copycols = rasops1_copycols;
		ri->ri_do_cursor = rasops1_do_cursor;
	}

	switch (ri->ri_font->fontwidth) {
#ifndef RASOPS_SMALL
	case 8:
		ri->ri_ops.putchar = rasops1_putchar8;
		break;
	case 16:
		ri->ri_ops.putchar = rasops1_putchar16;
		break;
#endif
	default:
		ri->ri_ops.putchar = rasops1_putchar;
		break;
	}
}

/*
 * Paint a single character. This is the generic version, this is ugly.
 */
static void
rasops1_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	uint32_t fs, rs, fb, bg, fg, lmask, rmask;
	uint32_t height, width;
	uint32_t *rp, *hrp, tmp, tmp0, tmp1;
	uint8_t *fr;
	bool space;

	hrp = NULL;	/* XXX GCC */

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	col *= ri->ri_font->fontwidth;
	rp = (uint32_t *)(ri->ri_bits + row * ri->ri_yscale +
	    ((col >> 3) & ~3));
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row * ri->ri_yscale +
		    ((col >> 3) & ~3));
	height = font->fontheight;
	width = font->fontwidth;
	col &= 31;
	rs = ri->ri_stride;

	bg = (attr & 0x000f0000) ? ri->ri_devcmap[1] : ri->ri_devcmap[0];
	fg = (attr & 0x0f000000) ? ri->ri_devcmap[1] : ri->ri_devcmap[0];

	/* If fg and bg match this becomes a space character */
	if (uc == ' ' || fg == bg) {
		space = true;
		fr = NULL;	/* XXX GCC */
		fs = 0;		/* XXX GCC */
	} else {
		space = false;
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;
	}

	if (col + width <= 32) {
		/* Single word, only one mask */

		rmask = rasops_pmask[col][width];
		lmask = ~rmask;

		if (space) {
			bg &= rmask;
			while (height--) {
				tmp = (*rp & lmask) | bg;
				*rp = tmp;
				DELTA(rp, rs, uint32_t *);
				if (ri->ri_hwbits) {
					*hrp = tmp;
					DELTA(hrp, rs, uint32_t *);
				}
			}
		} else {
			while (height--) {
				tmp = *rp & lmask;
				fb = fr[3] | (fr[2] << 8) |
				    (fr[1] << 16) | (fr[0] << 24);
				fr += fs;
				if (bg)
					fb = ~fb;
				tmp |= (MBE(fb >> col) & rmask);
				*rp = tmp;
				DELTA(rp, rs, uint32_t *);
				if (ri->ri_hwbits) {
					*hrp = tmp;
					DELTA(hrp, rs, uint32_t *);
				}
			}
		}

		/* Do underline */
		if ((attr & WSATTR_UNDERLINE) != 0) {
			DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
			tmp = (*rp & lmask) | (fg & rmask);
			*rp = tmp;
			if (ri->ri_hwbits) {
				DELTA(hrp, -(ri->ri_stride << 1), uint32_t *);
				*hrp = tmp;
			}
		}
	} else {
		/* Word boundary, two masks needed */

		lmask = ~rasops_lmask[col];
		rmask = ~rasops_rmask[(col + width) & 31];

		if (space) {
			width = bg & ~rmask;
			bg = bg & ~lmask;
			while (height--) {
				tmp0 = (rp[0] & lmask) | bg;
				tmp1 = (rp[1] & rmask) | width;
				rp[0] = tmp0;
				rp[1] = tmp1;
				DELTA(rp, rs, uint32_t *);
				if (ri->ri_hwbits) {
					hrp[0] = tmp0;
					hrp[1] = tmp1;
					DELTA(hrp, rs, uint32_t *);
				}
			}
		} else {
			width = 32 - col;
			while (height--) {
				tmp0 = rp[0] & lmask;
				tmp1 = rp[1] & rmask;
				fb = fr[3] | (fr[2] << 8) |
				    (fr[1] << 16) | (fr[0] << 24);
				fr += fs;
				if (bg)
					fb = ~fb;
				tmp0 |= MBE(fb >> col);
				tmp1 |= (MBE(fb << width) & ~rmask);
				rp[0] = tmp0;
				rp[1] = tmp1;
				DELTA(rp, rs, uint32_t *);
				if (ri->ri_hwbits) {
					hrp[0] = tmp0;
					hrp[1] = tmp1;
					DELTA(hrp, rs, uint32_t *);
				}
			}
		}

		/* Do underline */
		if ((attr & WSATTR_UNDERLINE) != 0) {
			DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
			tmp0 = (rp[0] & lmask) | (fg & ~lmask);
			tmp1 = (rp[1] & rmask) | (fg & ~rmask);
			rp[0] = tmp0;
			rp[1] = tmp1;
			if (ri->ri_hwbits) {
				DELTA(hrp, -(ri->ri_stride << 1), uint32_t *);
				hrp[0] = tmp0;
				hrp[1] = tmp1;
			}
		}
	}
}

#ifndef RASOPS_SMALL

#define	RASOPS_WIDTH	8
#include "rasops1_putchar_width.h"
#undef	RASOPS_WIDTH

#define	RASOPS_WIDTH	16
#include "rasops1_putchar_width.h"
#undef	RASOPS_WIDTH

#endif	/* !RASOPS_SMALL */

/*
 * Grab routines common to depths where (bpp < 8)
 */
#define NAME(ident)	rasops1_##ident
#define PIXEL_SHIFT	0

#include <dev/rasops/rasops_bitops.h>
