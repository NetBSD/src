/* $NetBSD: rasops_putchar_aa.h,v 1.1 2019/07/29 10:55:56 rin Exp $ */

/* NetBSD: rasops8.c,v 1.43 2019/07/28 12:06:10 rin Exp */
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

#if RASOPS_DEPTH != 8 && RASOPS_DEPTH != 15 && /* RASOPS_DEPTH != 24 && */ \
    RASOPS_DEPTH != 32
#error "Depth not supported"
#endif

#define	PUTCHAR_AA(depth)	PUTCHAR_AA1(depth)
#define	PUTCHAR_AA1(depth)	rasops ## depth ## _putchar_aa

#if   RASOPS_DEPTH == 8
#define	PIXEL_TYPE	uint8_t
#define	PIXEL_BITS	8
#elif RASOPS_DEPTH == 15
#define	PIXEL_TYPE	uint16_t
#define	PIXEL_BITS	16
#elif RASOPS_DEPTH == 32
#define	PIXEL_TYPE	uint32_t
#define	PIXEL_BITS	32
#endif

#define	MAX_WIDTH	64	/* XXX */

static void
PUTCHAR_AA(RASOPS_DEPTH)(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, width, bgo, fgo, x, y;
	uint8_t *fr, r0, r1, g0, g1, b0, b1, aval;
#if RASOPS_DEPTH == 8
	uint16_t r, g, b;
#else
	PIXEL_TYPE r, g, b;
#endif
	PIXEL_TYPE *rp, *hp, bg, fg, pixel;
	PIXEL_TYPE buf[MAX_WIDTH] __attribute__ ((aligned(8))); /* XXX */

	hp = NULL;	/* XXX GCC */

	if (!CHAR_IN_FONT(uc, font))
		return;

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	rp = (PIXEL_TYPE *)(ri->ri_bits + row * ri->ri_yscale +
	    col * ri->ri_xscale);
	if (ri->ri_hwbits)
		hp = (PIXEL_TYPE *)(ri->ri_hwbits + row * ri->ri_yscale +
		    col * ri->ri_xscale);
	
	height = font->fontheight;

	/* XXX */
	width = font->fontwidth;
	if (__predict_false(width > MAX_WIDTH))
		width = MAX_WIDTH;

	bg = (PIXEL_TYPE)ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf];
	fg = (PIXEL_TYPE)ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf];

	if (uc == ' ') {
#if RASOPS_DEPTH == 8
		memset(buf, bg, width);
#else
		for (x = 0; x < width; x++)
			buf[x] = bg;
#endif
		while (height--) {
			memcpy(rp, buf, width * sizeof(PIXEL_TYPE));
			DELTA(rp, ri->ri_stride, PIXEL_TYPE *);
			if (ri->ri_hwbits) {
				memcpy(hp, buf, width * sizeof(PIXEL_TYPE));
				DELTA(hp, ri->ri_stride, PIXEL_TYPE *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);

		/*
		 * This is independent to positions/lengths of RGB in pixel.
		 */
		bgo = (((uint32_t)attr >> 16) & 0xf) * 3;
		fgo = (((uint32_t)attr >> 24) & 0xf) * 3;

		r0 = rasops_cmap[bgo];
		r1 = rasops_cmap[fgo];
		g0 = rasops_cmap[bgo + 1];
		g1 = rasops_cmap[fgo + 1];
		b0 = rasops_cmap[bgo + 2];
		b1 = rasops_cmap[fgo + 2];

		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				aval = *fr;
				fr++;
				if (aval == 0) {
					pixel = bg;
				} else if (aval == 255) {
					pixel = fg;
				} else {
					r = aval * r1 + (0xff - aval) * r0;
					g = aval * g1 + (0xff - aval) * g0;
					b = aval * b1 + (0xff - aval) * b0;
#define	RGB2PIXEL(r, g, b)						\
	((((r) & 0xff00) >> (8 + 8 - ri->ri_rnum)) << ri->ri_rpos) |	\
	((((g) & 0xff00) >> (8 + 8 - ri->ri_gnum)) << ri->ri_gpos) |	\
	((((b) & 0xff00) >> (8 + 8 - ri->ri_bnum)) << ri->ri_bpos)
					pixel = RGB2PIXEL(r, g, b);
#undef	RGB2PIXEL
				}
				buf[x] = pixel;
			}
			memcpy(rp, buf, width * sizeof(PIXEL_TYPE));
			DELTA(rp, ri->ri_stride, PIXEL_TYPE *);
			if (ri->ri_hwbits) {
				memcpy(hp, buf, width * sizeof(PIXEL_TYPE));
				DELTA(hp, ri->ri_stride, PIXEL_TYPE *);
			}
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), PIXEL_TYPE *);
		if (ri->ri_hwbits) {
			DELTA(hp, -(ri->ri_stride << 1), PIXEL_TYPE *);
		}
		while (width--) {
			*rp++ = fg;
			if (ri->ri_hwbits)
				*hp++ = fg;
		}
	}
}

#undef	PUTCHAR_AA
#undef	PUTCHAR_AA1

#undef	PIXEL_TYPE
#undef	PIXEL_BITS

#undef	MAX_WIDTH
