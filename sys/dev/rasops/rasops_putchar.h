/* $NetBSD: rasops_putchar.h,v 1.2 2019/07/28 12:06:10 rin Exp $ */

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

#if RASOPS_DEPTH == 8
#define	CLR_TYPE	uint8_t
#elif RASOPS_DEPTH ==  15
#define	CLR_TYPE	uint16_t
#else
#define	CLR_TYPE	uint32_t
#endif

#if RASOPS_DEPTH == 24
#define	SUBST_CLR(p, index)						\
	do {								\
		CLR_TYPE c = clr[index];				\
		*(p)++ = c >> 16;					\
		*(p)++ = c >> 8;					\
		*(p)++ = c;						\
	} while (0 /* CONSTCOND */)
#else
#define	SUBST_CLR(p, index)						\
	do {								\
		*(CLR_TYPE *)(p) = clr[index];				\
		(p) += sizeof(CLR_TYPE);				\
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
	int width, height, cnt, fs, fb;
	uint8_t *dp, *rp, *hp, *hrp, *fr;
	CLR_TYPE clr[2];

	hp = hrp = NULL;	/* XXX GCC */

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
		hrp = ri->ri_hwbits + row * ri->ri_yscale + col *
		    ri->ri_xscale;

	height = font->fontheight;
	width = font->fontwidth;

	clr[0] = (CLR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf];
	clr[1] = (CLR_TYPE)ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf];

	if (uc == ' ') {
		while (height--) {
#if RASOPS_DEPTH == 8
			memset(rp, clr[0], width);
			if (ri->ri_hwbits)
				memset(hrp, clr[0], width);
#else
			dp = rp;
			if (ri->ri_hwbits)
				hp = hrp;
			for (cnt = width; cnt; cnt--) {
				SUBST_CLR(dp, 0);
				if (ri->ri_hwbits)
					SUBST_CLR(hp, 0);
			}
#endif
			rp += ri->ri_stride;
			if (ri->ri_hwbits)
				hrp += ri->ri_stride;
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			dp = rp;
			rp += ri->ri_stride;
			if (ri->ri_hwbits) {
				hp = hrp;
				hrp += ri->ri_stride;
			}
			fb = fr[3] | (fr[2] << 8) | (fr[1] << 16) |
			    (fr[0] << 24);
			fr += fs;
			for (cnt = width; cnt; cnt--) {
				SUBST_CLR(dp, (fb >> 31) & 1);
				if (ri->ri_hwbits)
					SUBST_CLR(hp, (fb >> 31) & 1);
				fb <<= 1;
			}
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		rp -= (ri->ri_stride << 1);
		if (ri->ri_hwbits)
			hrp -= (ri->ri_stride << 1);
#if RASOPS_DEPTH == 8
		memset(rp, clr[1], width);
		if (ri->ri_hwbits)
			memset(hrp, clr[1], width);
#else
		while (width--) {
			SUBST_CLR(rp, 1);
			if (ri->ri_hwbits)
				SUBST_CLR(hrp, 1);
		}
#endif
	}
}

#undef	PUTCHAR
#undef	CLR_TYPE
#undef	SUBST_CLR
