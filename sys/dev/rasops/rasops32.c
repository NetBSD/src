/*	 $NetBSD: rasops32.c,v 1.36 2019/07/25 15:12:47 rin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rasops32.c,v 1.36 2019/07/25 15:12:47 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops32_putchar(void *, int, int, u_int, long attr);
static void 	rasops32_putchar_aa(void *, int, int, u_int, long attr);
#ifndef RASOPS_SMALL
static void	rasops32_putchar8(void *, int, int, u_int, long);
static void	rasops32_putchar12(void *, int, int, u_int, long);
static void	rasops32_putchar16(void *, int, int, u_int, long);
static void	rasops32_makestamp(struct rasops_info *, long);

/*
 * 4x1 stamp for optimized character blitting
 */
static uint32_t	stamp[64];
static long	stamp_attr;
static int	stamp_mutex;	/* XXX see note in readme */
#endif

/*
 * offset = STAMP_SHIFT(fontbits, nibble #) & STAMP_MASK
 * destination uint32_t[0] = STAMP_READ(offset)
 * destination uint32_t[1] = STAMP_READ(offset +  4)
 * destination uint32_t[2] = STAMP_READ(offset +  8)
 * destination uint32_t[3] = STAMP_READ(offset + 12)
 */
#define	STAMP_SHIFT(fb, n)	((n) ? (fb) : (fb) << 4)
#define	STAMP_MASK		(0xf << 4)
#define	STAMP_READ(o)		(*(uint32_t *)((char *)stamp + (o)))

/*
 * Initialize a 'rasops_info' descriptor for this depth.
 */
void
rasops32_init(struct rasops_info *ri)
{

	if (ri->ri_rnum == 0) {
		ri->ri_rnum = 8;
		ri->ri_rpos = 0;
		ri->ri_gnum = 8;
		ri->ri_gpos = 8;
		ri->ri_bnum = 8;
		ri->ri_bpos = 16;
	}

	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = rasops32_putchar_aa;
		return;
	}

	switch (ri->ri_font->fontwidth) {
#ifndef RASOPS_SMALL
	case 8:
		ri->ri_ops.putchar = rasops32_putchar8;
		break;
	case 12:
		ri->ri_ops.putchar = rasops32_putchar12;
		break;
	case 16:
		ri->ri_ops.putchar = rasops32_putchar16;
		break;
#endif
	default:
		ri->ri_ops.putchar = rasops32_putchar;
		break;
	}
}

/*
 * Paint a single character.
 */

static void
rasops32_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	int width, height, cnt, fs, fb, clr[2];
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	uint32_t *dp, *rp, *hp, *hrp;
	uint8_t *fr;

	hp = hrp = NULL;

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	/* check if character fits into font limits */
	if (!CHAR_IN_FONT(uc, font))
		return;

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row*ri->ri_yscale +
		    col*ri->ri_xscale);

	height = font->fontheight;
	width = font->fontwidth;

	clr[0] = ri->ri_devcmap[(attr >> 16) & 0xf];
	clr[1] = ri->ri_devcmap[(attr >> 24) & 0xf];

	if (uc == ' ') {
		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				hp = hrp;
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}

			for (cnt = width; cnt; cnt--) {
				*dp++ = clr[0];
				if (ri->ri_hwbits)
					*hp++ = clr[0];
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			dp = rp;
			fb = fr[3] | (fr[2] << 8) | (fr[1] << 16) |
			    (fr[0] << 24);
			fr += fs;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				hp = hrp;
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}

			for (cnt = width; cnt; cnt--) {
				*dp++ = clr[(fb >> 31) & 1];
				if (ri->ri_hwbits)
					*hp++ = clr[(fb >> 31) & 1];
				fb <<= 1;
			}
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
		if (ri->ri_hwbits)
			DELTA(hrp, -(ri->ri_stride << 1), uint32_t *);

		while (width--) {
			*rp++ = clr[1];
			if (ri->ri_hwbits)
				*hrp++ = clr[1];
		}
	}
}

static void
rasops32_putchar_aa(void *cookie, int row, int col, u_int uc, long attr)
{
	int width, height, cnt, clr[2];
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	uint32_t *dp, *rp;
	uint8_t *rrp;
	uint8_t *fr;
	uint32_t buffer[64]; /* XXX */
	int x, y, r, g, b, aval;
	int r1, g1, b1, r0, g0, b0;

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	/* check if character fits into font limits */
	if (!CHAR_IN_FONT(uc, font))
		return;

	rrp = (ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	rp = (uint32_t *)rrp;

	height = font->fontheight;
	width = font->fontwidth;

	clr[0] = ri->ri_devcmap[(attr >> 16) & 0xf];
	clr[1] = ri->ri_devcmap[(attr >> 24) & 0xf];

	if (uc == ' ') {
		for (cnt = 0; cnt < width; cnt++)
			buffer[cnt] = clr[0];
		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, uint32_t *);
			memcpy(dp, buffer, width << 2);
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);

		r0 = (clr[0] >> 16) & 0xff;
		r1 = (clr[1] >> 16) & 0xff;
		g0 = (clr[0] >> 8) & 0xff;
		g1 = (clr[1] >> 8) & 0xff;
		b0 =  clr[0] & 0xff;
		b1 =  clr[1] & 0xff;

		for (y = 0; y < height; y++) {
			dp = (uint32_t *)(rrp + ri->ri_stride * y);
			for (x = 0; x < width; x++) {
				aval = *fr;
				if (aval == 0) {
					buffer[x] = clr[0];
				} else if (aval == 255) {
					buffer[x] = clr[1];
				} else {
					r = aval * r1 + (255 - aval) * r0;
					g = aval * g1 + (255 - aval) * g0;
					b = aval * b1 + (255 - aval) * b0;
					buffer[x] = (r & 0xff00) << 8 |
					      (g & 0xff00) |
					      (b & 0xff00) >> 8;
				}
				fr++;
			}
			memcpy(dp, buffer, width << 2);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		rp = (uint32_t *)rrp;
		height = font->fontheight;
		DELTA(rp, (ri->ri_stride * (height - 2)), uint32_t *);
		while (width--)
			*rp++ = clr[1];
	}
}

#ifndef RASOPS_SMALL
/*
 * Recompute the blitting stamp.
 */
static void
rasops32_makestamp(struct rasops_info *ri, long attr)
{
	uint32_t fg, bg;
	int i;

	fg = ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf];
	bg = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf];
	stamp_attr = attr;

	for (i = 0; i < 64; i += 4) {
		stamp[i + 0] = (i & 32 ? fg : bg);
		stamp[i + 1] = (i & 16 ? fg : bg);
		stamp[i + 2] = (i & 8 ? fg : bg);
		stamp[i + 3] = (i & 4 ? fg : bg);
	}
}

/*
 * Put a single character. This is for 8-pixel wide fonts.
 */
static void
rasops32_putchar8(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, so, fs;
	uint32_t *rp, *hrp = NULL;
	uint8_t *fr;

	hrp = NULL; /* XXX GCC */

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	/* check if character fits into font limits */
	if (!CHAR_IN_FONT(uc, font))
		return;

	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops32_putchar(cookie, row, col, uc, attr);
		return;
	}

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops32_makestamp(ri, attr);

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row*ri->ri_yscale +
		    col*ri->ri_xscale);

	height = font->fontheight;

	if (uc == ' ') {
		while (height--) {
			rp[0] = rp[1] = rp[2] = rp[3] =
			rp[4] = rp[5] = rp[6] = rp[7] = stamp[0];
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				hrp[0] = hrp[1] = hrp[2] = hrp[3] =
				hrp[4] = hrp[5] = hrp[6] = hrp[7] = stamp[0];
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			uint32_t tmp0, tmp1, tmp2, tmp3;

			so = STAMP_SHIFT(fr[0], 1) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[0] = tmp0;
			rp[1] = tmp1;
			rp[2] = tmp2;
			rp[3] = tmp3;
			if (ri->ri_hwbits) {
				hrp[0] = tmp0;
				hrp[1] = tmp1;
				hrp[2] = tmp2;
				hrp[3] = tmp3;
			}

			so = STAMP_SHIFT(fr[0], 0) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[4] = tmp0;
			rp[5] = tmp1;
			rp[6] = tmp2;
			rp[7] = tmp3;
			if (ri->ri_hwbits) {
				hrp[4] = tmp0;
				hrp[5] = tmp1;
				hrp[6] = tmp2;
				hrp[7] = tmp3;
			}

			fr += fs;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits)
				DELTA(hrp, ri->ri_stride, uint32_t *);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
		rp[0] = rp[1] = rp[2] = rp[3] =
		rp[4] = rp[5] = rp[6] = rp[7] = stamp[60];
		if (ri->ri_hwbits)
			hrp[0] = hrp[1] = hrp[2] = hrp[3] =
			hrp[4] = hrp[5] = hrp[6] = hrp[7] = stamp[60];
	}

	stamp_mutex--;
}

/*
 * Put a single character. This is for 12-pixel wide fonts.
 */
static void
rasops32_putchar12(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, so, fs;
	uint32_t *rp, *hrp = NULL;
	uint8_t *fr;

	hrp = NULL; /* XXX GCC */

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	/* check if character fits into font limits */
	if (!CHAR_IN_FONT(uc, font))
		return;

	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops32_putchar(cookie, row, col, uc, attr);
		return;
	}

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops32_makestamp(ri, attr);

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row*ri->ri_yscale +
		    col*ri->ri_xscale);

	height = font->fontheight;

	if (uc == ' ') {
		while (height--) {
			rp[0]  = rp[1]  = rp[2]  = rp[3]  =
			rp[4]  = rp[5]  = rp[6]  = rp[7]  =
			rp[8]  = rp[9]  = rp[10] = rp[11] = stamp[0];
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				hrp[0]  = hrp[1]  = hrp[2]  = hrp[3]  =
				hrp[4]  = hrp[5]  = hrp[6]  = hrp[7]  =
				hrp[8]  = hrp[9]  = hrp[10] = hrp[11] =
				    stamp[0];
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			uint32_t tmp0, tmp1, tmp2, tmp3;

			so = STAMP_SHIFT(fr[0], 1) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[0] = tmp0;
			rp[1] = tmp1;
			rp[2] = tmp2;
			rp[3] = tmp3;
			if (ri->ri_hwbits) {
				hrp[0] = tmp0;
				hrp[1] = tmp1;
				hrp[2] = tmp2;
				hrp[3] = tmp3;
			}

			so = STAMP_SHIFT(fr[0], 0) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[4] = tmp0;
			rp[5] = tmp1;
			rp[6] = tmp2;
			rp[7] = tmp3;
			if (ri->ri_hwbits) {
				hrp[4] = tmp0;
				hrp[5] = tmp1;
				hrp[6] = tmp2;
				hrp[7] = tmp3;
			}

			so = STAMP_SHIFT(fr[1], 1) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[8] = tmp0;
			rp[9] = tmp1;
			rp[10] = tmp2;
			rp[11] = tmp3;
			if (ri->ri_hwbits) {
				hrp[8] = tmp0;
				hrp[9] = tmp1;
				hrp[10] = tmp2;
				hrp[11] = tmp3;
			}

			fr += fs;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits)
				DELTA(hrp, ri->ri_stride, uint32_t *);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
		rp[0]  = rp[1]  = rp[2]  = rp[3]  =
		rp[4]  = rp[5]  = rp[6]  = rp[7]  =
		rp[8]  = rp[9]  = rp[10] = rp[11] = stamp[60];
		if (ri->ri_hwbits)
			hrp[0]  = hrp[1]  = hrp[2]  = hrp[3]  =
			hrp[4]  = hrp[5]  = hrp[6]  = hrp[7]  =
			hrp[8]  = hrp[9]  = hrp[10] = hrp[11] = stamp[60];
	}

	stamp_mutex--;
}

/*
 * Put a single character. This is for 16-pixel wide fonts.
 */
static void
rasops32_putchar16(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, so, fs;
	uint32_t *rp, *hrp = NULL;
	uint8_t *fr;

	hrp = NULL; /* XXX GCC */

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	/* check if character fits into font limits */
	if (!CHAR_IN_FONT(uc, font))
		return;

	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops32_putchar(cookie, row, col, uc, attr);
		return;
	}

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops32_makestamp(ri, attr);

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row*ri->ri_yscale +
		    col*ri->ri_xscale);

	height = font->fontheight;

	if (uc == ' ') {
		while (height--) {
			rp[0]  = rp[1]  = rp[2]  = rp[3]  =
			rp[4]  = rp[5]  = rp[6]  = rp[7]  =
			rp[8]  = rp[9]  = rp[10] = rp[11] =
			rp[12] = rp[13] = rp[14] = rp[15] = stamp[0];
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				hrp[0]  = hrp[1]  = hrp[2]  = hrp[3]  =
				hrp[4]  = hrp[5]  = hrp[6]  = hrp[7]  =
				hrp[8]  = hrp[9]  = hrp[10] = hrp[11] =
				hrp[12] = hrp[13] = hrp[14] = hrp[15] =
				    stamp[0];
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			uint32_t tmp0, tmp1, tmp2, tmp3;

			so = STAMP_SHIFT(fr[0], 1) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[0] = tmp0;
			rp[1] = tmp1;
			rp[2] = tmp2;
			rp[3] = tmp3;
			if (ri->ri_hwbits) {
				hrp[0] = tmp0;
				hrp[1] = tmp1;
				hrp[2] = tmp2;
				hrp[3] = tmp3;
			}

			so = STAMP_SHIFT(fr[0], 0) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[4] = tmp0;
			rp[5] = tmp1;
			rp[6] = tmp2;
			rp[7] = tmp3;
			if (ri->ri_hwbits) {
				hrp[4] = tmp0;
				hrp[5] = tmp1;
				hrp[6] = tmp2;
				hrp[7] = tmp3;
			}

			so = STAMP_SHIFT(fr[1], 1) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[8] = tmp0;
			rp[9] = tmp1;
			rp[10] = tmp2;
			rp[11] = tmp3;
			if (ri->ri_hwbits) {
				hrp[8] = tmp0;
				hrp[9] = tmp1;
				hrp[10] = tmp2;
				hrp[11] = tmp3;
			}

			so = STAMP_SHIFT(fr[1], 0) & STAMP_MASK;
			tmp0 = STAMP_READ(so);
			tmp1 = STAMP_READ(so +  4);
			tmp2 = STAMP_READ(so +  8);
			tmp3 = STAMP_READ(so + 12);
			rp[12] = tmp0;
			rp[13] = tmp1;
			rp[14] = tmp2;
			rp[15] = tmp3;
			if (ri->ri_hwbits) {
				hrp[12] = tmp0;
				hrp[13] = tmp1;
				hrp[14] = tmp2;
				hrp[15] = tmp3;
			}

			fr += fs;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits)
				DELTA(hrp, ri->ri_stride, uint32_t *);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
		rp[0]  = rp[1]  = rp[2]  = rp[3]  =
		rp[4]  = rp[5]  = rp[6]  = rp[7]  =
		rp[8]  = rp[9]  = rp[10] = rp[11] =
		rp[12] = rp[13] = rp[14] = rp[15] = stamp[60];
		if (ri->ri_hwbits)
			hrp[0]  = hrp[1]  = hrp[2]  = hrp[3]  =
			hrp[4]  = hrp[5]  = hrp[6]  = hrp[7]  =
			hrp[8]  = hrp[9]  = hrp[10] = hrp[11] =
			hrp[12] = hrp[13] = hrp[14] = hrp[15] = stamp[60];
	}

	stamp_mutex--;
}
#endif
