/* $NetBSD: rasops_putchar.h,v 1.5 2019/07/30 15:29:40 rin Exp $ */

/* NetBSD: rasops8.c,v 1.41 2019/07/25 03:02:44 rin Exp  */
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

#define PUTCHAR(depth)	PUTCHAR1(depth)
#define PUTCHAR1(depth)	rasops ## depth ## _putchar

#if   RASOPS_DEPTH == 8
#define	COLOR_TYPE		uint8_t
#elif RASOPS_DEPTH ==  15
#define	COLOR_TYPE		uint16_t
#else
#define	COLOR_TYPE		uint32_t
#endif

#if RASOPS_DEPTH != 24
#define	PIXEL_BYTES		sizeof(COLOR_TYPE)
#define	SET_PIXEL(p, index)						\
	do {								\
		*(COLOR_TYPE *)(p) = clr[index];			\
		(p) += sizeof(COLOR_TYPE);				\
	} while (0 /* CONSTCOND */)
#endif

#if RASOPS_DEPTH == 24
#define	PIXEL_BYTES		3
#define	SET_PIXEL(p, index)						\
	do {								\
		COLOR_TYPE c = clr[index];				\
		*(p)++ = c >> 16;					\
		*(p)++ = c >> 8;					\
		*(p)++ = c;						\
	} while (0 /* CONSTCOND */)
#endif

/*
 * Put a single character.
 */
static void
PUTCHAR(RASOPS_DEPTH)(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int width, height, cnt, fs;
	uint32_t fb;
	uint8_t *dp, *rp, *hp, *fr;
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

	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	if (ri->ri_hwbits)
		hp = ri->ri_hwbits + row * ri->ri_yscale + col * ri->ri_xscale;

	height = font->fontheight;
	width = font->fontwidth;

	clr[0] = (COLOR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf];
	clr[1] = (COLOR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf];

	if (uc == ' ') {
		while (height--) {
			dp = rp;
			for (cnt = width; cnt; cnt--)
				SET_PIXEL(dp, 0);
			if (ri->ri_hwbits) {
				uint16_t bytes = width * PIXEL_BYTES;
					/* XXX GCC */
				memcpy(hp, rp, bytes);
				hp += ri->ri_stride;
			}
			rp += ri->ri_stride;
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			dp = rp;
			fb = be32uatoh(fr);
			fr += fs;
			for (cnt = width; cnt; cnt--) {
				SET_PIXEL(dp, (fb >> 31) & 1);
				fb <<= 1;
			}
			if (ri->ri_hwbits) {
				uint16_t bytes = width * PIXEL_BYTES;
					/* XXX GCC */
				memcpy(hp, rp, bytes);
				hp += ri->ri_stride;
			}
			rp += ri->ri_stride;
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		rp -= (ri->ri_stride << 1);
		dp = rp;
		while (width--)
			SET_PIXEL(dp, 1);
		if (ri->ri_hwbits) {
			hp -= (ri->ri_stride << 1);
			uint16_t bytes = width * PIXEL_BYTES; /* XXX GCC */
			memcpy(hp, rp, bytes);
		}
	}
}

#undef	PUTCHAR
#undef	COLOR_TYPE
#undef	PIXEL_BYTES
#undef	SET_PIXEL
