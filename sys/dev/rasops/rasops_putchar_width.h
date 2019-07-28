/* $NetBSD: rasops_putchar_width.h,v 1.6 2019/07/28 12:06:10 rin Exp $ */

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

#if RASOPS_WIDTH != 8 && RASOPS_WIDTH != 12 && RASOPS_WIDTH != 16
#error "Width not supported"
#endif

#if   RASOPS_DEPTH == 8
#define	FILLED_STAMP	15
#elif RASOPS_DEPTH == 15
#define	FILLED_STAMP	30
#else
#define	FILLED_STAMP	60
#endif

#define	PUTCHAR_WIDTH1(depth, width)	rasops ## depth ## _putchar ## width
#define	PUTCHAR_WIDTH(depth, width)	PUTCHAR_WIDTH1(depth, width)

#define	PUTCHAR1(depth)			rasops ## depth ## _putchar
#define	PUTCHAR(depth)			PUTCHAR1(depth)

#define	MAKESTAMP1(depth)		rasops ## depth ## _makestamp
#define	MAKESTAMP(depth)		MAKESTAMP1(depth)

/* ################################################################### */

#if RASOPS_DEPTH == 8

#define	SUBST_STAMP1(p, off, base)					\
	(p)[(off) * 1 + 0] = stamp[base]

#define	SUBST_GLYPH1(index, nibble, off)				\
	do {								\
		int so = STAMP_SHIFT(fr[index], nibble) & STAMP_MASK;	\
		rp[(off) * 1 + 0] = STAMP_READ(so);			\
		if (ri->ri_hwbits) {					\
			hrp[(off) * 1 + 0] = STAMP_READ(so);		\
		}							\
	} while (0 /* CONSTCOND */)

#endif /* RASOPS_DEPTH == 8 */

/* ################################################################### */

#if RASOPS_DEPTH == 15

#define	SUBST_STAMP1(p, off, base)					\
	(p)[(off) * 2 + 0] = (p)[(off) * 2 + 1] = stamp[base]

#define	SUBST_GLYPH1(index, nibble, off)				\
	do {								\
		int so = STAMP_SHIFT(fr[index], nibble) & STAMP_MASK;	\
		rp[(off) * 2 + 0] = STAMP_READ(so);			\
		rp[(off) * 2 + 1] = STAMP_READ(so +  4);		\
		if (ri->ri_hwbits) {					\
			hrp[(off) * 2 + 0] = STAMP_READ(so);		\
			hrp[(off) * 2 + 1] = STAMP_READ(so +  4);	\
		}							\
	} while (0 /* CONSTCOND */)

#endif /* RASOPS_DEPTH == 15 */

/* ################################################################### */

#if RASOPS_DEPTH == 24

#define	SUBST_STAMP1(p, off, base)					\
	do {								\
		(p)[(off) * 3 + 0] = stamp[(base) + 0];			\
		(p)[(off) * 3 + 1] = stamp[(base) + 1];			\
		(p)[(off) * 3 + 2] = stamp[(base) + 2];			\
	} while (0 /* CONSTCOND */)

#define	SUBST_GLYPH1(index, nibble, off)				\
	do {								\
		int so = STAMP_SHIFT(fr[index], nibble) & STAMP_MASK;	\
		rp[(off) * 3 + 0] = STAMP_READ(so);			\
		rp[(off) * 3 + 1] = STAMP_READ(so +  4);		\
		rp[(off) * 3 + 2] = STAMP_READ(so +  8);		\
		if (ri->ri_hwbits) {					\
			hrp[(off) * 3 + 0] = STAMP_READ(so);		\
			hrp[(off) * 3 + 1] = STAMP_READ(so +  4);	\
			hrp[(off) * 3 + 2] = STAMP_READ(so +  8);	\
		}							\
	} while (0 /* CONSTCOND */)

#endif /* RASOPS_DEPTH == 24 */

/* ################################################################### */

#if RASOPS_DEPTH == 32

#define	SUBST_STAMP1(p, off, base)					\
	(p)[(off) * 4 + 0] = (p)[(off) * 4 + 1] =			\
	(p)[(off) * 4 + 2] = (p)[(off) * 4 + 3] = stamp[base]

#define	SUBST_GLYPH1(index, nibble, off)				\
	do {								\
		int so = STAMP_SHIFT(fr[index], nibble) & STAMP_MASK;	\
		rp[(off) * 4 + 0] = STAMP_READ(so);			\
		rp[(off) * 4 + 1] = STAMP_READ(so +  4);		\
		rp[(off) * 4 + 2] = STAMP_READ(so +  8);		\
		rp[(off) * 4 + 3] = STAMP_READ(so + 12);		\
		if (ri->ri_hwbits) {					\
			hrp[(off) * 4 + 0] = STAMP_READ(so);		\
			hrp[(off) * 4 + 1] = STAMP_READ(so +  4);	\
			hrp[(off) * 4 + 2] = STAMP_READ(so +  8);	\
			hrp[(off) * 4 + 3] = STAMP_READ(so + 12);	\
		}							\
	} while (0 /* CONSTCOND */)

#endif /* RASOPS_DEPTH == 32 */

/* ################################################################### */

#if   RASOPS_WIDTH == 8
#define	SUBST_STAMP(p, base) 			\
	do {					\
		SUBST_STAMP1(p, 0, base);	\
		SUBST_STAMP1(p, 1, base);	\
	} while (0 /* CONSTCOND */)
#elif RASOPS_WIDTH == 12
#define	SUBST_STAMP(p, base)			\
	do {					\
		SUBST_STAMP1(p, 0, base);	\
		SUBST_STAMP1(p, 1, base);	\
		SUBST_STAMP1(p, 2, base);	\
	} while (0 /* CONSTCOND */)
#elif RASOPS_WIDTH == 16
#define	SUBST_STAMP(p, base)			\
	do {					\
		SUBST_STAMP1(p, 0, base);	\
		SUBST_STAMP1(p, 1, base);	\
		SUBST_STAMP1(p, 2, base);	\
		SUBST_STAMP1(p, 3, base);	\
	} while (0 /* CONSTCOND */)
#endif

#if   RASOPS_WIDTH == 8
#define	SUBST_GLYPH				\
	do {					\
		SUBST_GLYPH1(0, 1, 0);		\
		SUBST_GLYPH1(0, 0, 1);		\
	} while (0 /* CONSTCOND */)
#elif RASOPS_WIDTH == 12
#define	SUBST_GLYPH				\
	do {					\
		SUBST_GLYPH1(0, 1, 0);		\
		SUBST_GLYPH1(0, 0, 1);		\
		SUBST_GLYPH1(1, 1, 2);		\
	} while (0 /* CONSTCOND */)
#elif RASOPS_WIDTH == 16
#define	SUBST_GLYPH				\
	do {					\
		SUBST_GLYPH1(0, 1, 0);		\
		SUBST_GLYPH1(0, 0, 1);		\
		SUBST_GLYPH1(1, 1, 2);		\
		SUBST_GLYPH1(1, 0, 3);		\
	} while (0 /* CONSTCOND */)
#endif

/*
 * Width-optimized putchar function.
 */
static void
PUTCHAR_WIDTH(RASOPS_DEPTH, RASOPS_WIDTH)(void *cookie, int row, int col,
    u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, fs;
	uint32_t *rp, *hrp;
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
		PUTCHAR(RASOPS_DEPTH)(cookie, row, col, uc, attr);
		return;
	}

	/* Recompute stamp? */
	if (attr != stamp_attr)
		MAKESTAMP(RASOPS_DEPTH)(ri, attr);

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	if (ri->ri_hwbits)
		hrp = (uint32_t *)(ri->ri_hwbits + row*ri->ri_yscale +
		    col*ri->ri_xscale);

	height = font->fontheight;

	if (uc == ' ') {
		while (height--) {
			SUBST_STAMP(rp, 0);
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits) {
				SUBST_STAMP(hrp, 0);
				DELTA(hrp, ri->ri_stride, uint32_t *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			SUBST_GLYPH;

			fr += fs;
			DELTA(rp, ri->ri_stride, uint32_t *);
			if (ri->ri_hwbits)
				DELTA(hrp, ri->ri_stride, uint32_t *);
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), uint32_t *);
		SUBST_STAMP(rp, FILLED_STAMP);
		if (ri->ri_hwbits) {
			DELTA(hrp, -(ri->ri_stride << 1), uint32_t *);
			SUBST_STAMP(hrp, FILLED_STAMP);
		}
	}

	stamp_mutex--;
}

#undef	FILLED_STAMP

#undef	PUTCHAR_WIDTH1
#undef	PUTCHAR_WIDTH

#undef	PUTCHAR1
#undef	PUTCHAR

#undef	MAKESTAMP1
#undef	MAKESTAMP

#undef	SUBST_STAMP1
#undef	SUBST_STAMP

#undef	SUBST_GLYPH1
#undef	SUBST_GLYPH
