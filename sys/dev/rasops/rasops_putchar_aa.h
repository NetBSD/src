/* $NetBSD: rasops_putchar_aa.h,v 1.5 2019/07/31 02:26:40 rin Exp $ */

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

#if RASOPS_DEPTH != 8 && RASOPS_DEPTH != 15 && RASOPS_DEPTH != 24 && \
    RASOPS_DEPTH != 32
#error "Depth not supported"
#endif

#define	PUTCHAR_AA(depth)	PUTCHAR_AA1(depth)
#define	PUTCHAR_AA1(depth)	rasops ## depth ## _putchar_aa

#define	MAX_WIDTH		64	/* XXX */

#if   RASOPS_DEPTH == 8
#define	PIXEL_TYPE		uint8_t
#elif RASOPS_DEPTH == 15
#define	PIXEL_TYPE		uint16_t
#elif RASOPS_DEPTH == 24
#define	PIXEL_TYPE		uint8_t
#elif RASOPS_DEPTH == 32
#define	PIXEL_TYPE		uint32_t
#endif

#if RASOPS_DEPTH != 24
#define	COLOR_TYPE		PIXEL_TYPE
#define	PIXEL_BYTES		sizeof(PIXEL_TYPE)
#define	BUF_LEN			MAX_WIDTH
#define	SET_PIXEL(p, x, c)	(p)[x] = clr[c]
#endif

#if RASOPS_DEPTH == 24
#define	COLOR_TYPE		uint32_t
#define	PIXEL_BYTES		3
#define	BUF_LEN			(MAX_WIDTH * 3)
#define	SET_PIXEL(p, x, c)			\
	do {					\
		(p)[3 * x + 0] = clr[c] >> 16;	\
		(p)[3 * x + 1] = clr[c] >> 8;	\
		(p)[3 * x + 2] = clr[c];	\
	} while (0 /* CONSTCOND */)
#endif

#if RASOPS_DEPTH != 8
#define	SET_WIDTH(p, c)	for (x = 0; x < width; x++) { SET_PIXEL(p, x, c); }
#else
#define	SET_WIDTH(p, c)	memset(p, clr[c], width)
#endif

static void
PUTCHAR_AA(RASOPS_DEPTH)(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, width, x, y, off[2];
	uint16_t r[2], g[2], b[2];
	uint8_t *fr, aval;
	PIXEL_TYPE *rp, *hp, R, G, B;
	PIXEL_TYPE buf[BUF_LEN] __attribute__ ((aligned(8))); /* XXX */
	COLOR_TYPE clr[2];

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

	clr[0] = (COLOR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf];
	clr[1] = (COLOR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf];

	if (uc == ' ') {
		SET_WIDTH(buf, 0);
		while (height--) {
			memcpy(rp, buf, width * PIXEL_BYTES);
			DELTA(rp, ri->ri_stride, PIXEL_TYPE *);
			if (ri->ri_hwbits) {
				memcpy(hp, buf, width * PIXEL_BYTES);
				DELTA(hp, ri->ri_stride, PIXEL_TYPE *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);

		/*
	 	 * This is independent to positions/lengths of RGB in pixel.
	 	 */
		off[0] = (((uint32_t)attr >> 16) & 0xf) * 3;
		off[1] = (((uint32_t)attr >> 24) & 0xf) * 3;

		r[0] = rasops_cmap[off[0]];
		r[1] = rasops_cmap[off[1]];
		g[0] = rasops_cmap[off[0] + 1];
		g[1] = rasops_cmap[off[1] + 1];
		b[0] = rasops_cmap[off[0] + 2];
		b[1] = rasops_cmap[off[1] + 2];

		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				aval = *fr;
				fr++;
				if (aval == 0)
					SET_PIXEL(rp, x, 0);
				else if (aval == 255)
					SET_PIXEL(rp, x, 1);
				else {
#define	AVERAGE(p, w)	((w * p[1] + (0xff - w) * p[0]) >> 8)
					R = AVERAGE(r, aval);
					G = AVERAGE(g, aval);
					B = AVERAGE(b, aval);
#undef	AVERAGE

#if RASOPS_DEPTH != 24
#define	RGB2PIXEL(_r, _g, _b)				\
	(((_r) >> (8 - ri->ri_rnum)) << ri->ri_rpos) |	\
	(((_g) >> (8 - ri->ri_gnum)) << ri->ri_gpos) |	\
	(((_b) >> (8 - ri->ri_bnum)) << ri->ri_bpos)
					rp[x] = RGB2PIXEL(R, G, B);
#undef	RGB2PIXEL
#endif

#if RASOPS_DEPTH == 24
#  if BYTE_ORDER == LITTLE_ENDIAN
#define	ROFF	(ri->ri_rpos / 8)
#define	GOFF	(ri->ri_gpos / 8)
#define	BOFF	(ri->ri_bpos / 8)
#  else	/* BIG_ENDIAN XXX not tested */
#define	ROFF	(2 - ri->ri_rpos / 8)
#define	GOFF	(2 - ri->ri_gpos / 8)
#define	BOFF	(2 - ri->ri_bpos / 8)
#  endif
					rp[3 * x + ROFF] = R;
					rp[3 * x + GOFF] = G;
					rp[3 * x + BOFF] = B;
#undef	ROFF
#undef	GOFF
#undef	BOFF
#endif
				}
			}
			if (ri->ri_hwbits) {
				memcpy(hp, rp, width * PIXEL_BYTES);
				DELTA(hp, ri->ri_stride, PIXEL_TYPE *);
			}
			DELTA(rp, ri->ri_stride, PIXEL_TYPE *);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), PIXEL_TYPE *);
		SET_WIDTH(rp, 1);
		if (ri->ri_hwbits) {
			DELTA(hp, -(ri->ri_stride << 1), PIXEL_TYPE *);
			memcpy(hp, rp, width * PIXEL_BYTES);
		}
	}
}

#undef	PUTCHAR_AA
#undef	PUTCHAR_AA1

#undef	MAX_WIDTH

#undef	PIXEL_TYPE
#undef	COLOR_TYPE
#undef	PIXEL_BYTES
#undef	SET_PIXEL
#undef	SET_WIDTH
