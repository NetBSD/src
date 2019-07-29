/* $NetBSD: rasops1_putchar_width.h,v 1.2 2019/07/29 17:22:19 rin Exp $ */

/* NetBSD: rasops1.c,v 1.28 2019/07/25 03:02:44 rin Exp */
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

#if RASOPS_WIDTH != 8 && RASOPS_WIDTH != 16
#error "Width not supported"
#endif

#define	PUTCHAR_WIDTH1(width)	rasops1_putchar ## width
#define	PUTCHAR_WIDTH(width)	PUTCHAR_WIDTH1(width)

#if RASOPS_WIDTH == 8
#define	COPY_UNIT	uint8_t
#define	GET_GLYPH	tmp = fr[0]
#endif

#if RASOPS_WIDTH == 16
/*
 * rp and hrp are always half-word aligned, whereas
 * fr may not be aligned in half-word boundary.
 */
#define	COPY_UNIT	uint16_t
#  if BYTE_ORDER == BIG_ENDIAN
#define	GET_GLYPH	tmp = (fr[0] << 8) | fr[1]
#  else
#define	GET_GLYPH	tmp = fr[0] | (fr[1] << 8)
#  endif
#endif /* RASOPS_WIDTH == 16 */

/*
 * Width-optimized putchar function.
 */
static void
PUTCHAR_WIDTH(RASOPS_WIDTH)(void *cookie, int row, int col, u_int uc, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, uc);
	int height, fs, rs, bg, fg;
	uint8_t *fr;
	COPY_UNIT *rp, *hrp, tmp;

	hrp = NULL;	/* XXX GCC */

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif

	rp = (COPY_UNIT *)(ri->ri_bits + row * ri->ri_yscale +
	    col * sizeof(COPY_UNIT));
	if (ri->ri_hwbits)
		hrp = (COPY_UNIT *)(ri->ri_hwbits + row * ri->ri_yscale +
		    col * sizeof(COPY_UNIT));
	height = font->fontheight;
	rs = ri->ri_stride;

	bg = (attr & 0x000f0000) ? ri->ri_devcmap[1] : ri->ri_devcmap[0];
	fg = (attr & 0x0f000000) ? ri->ri_devcmap[1] : ri->ri_devcmap[0];

	/* If fg and bg match this becomes a space character */
	if (uc == ' ' || fg == bg) {
		while (height--) {
			*rp = bg;
			DELTA(rp, rs, COPY_UNIT *);
			if (ri->ri_hwbits) {
				*hrp = bg;
				DELTA(hrp, rs, COPY_UNIT *);
			}
		}
	} else {
		fr = FONT_GLYPH(uc, font, ri);
		fs = font->stride;

		while (height--) {
			GET_GLYPH;
			if (bg)
				tmp = ~tmp;
			*rp = tmp;
			DELTA(rp, rs, COPY_UNIT *);
			if (ri->ri_hwbits) {
				*hrp = tmp;
				DELTA(hrp, rs, COPY_UNIT *);
			}
			fr += fs;
		}
	}

	/* Do underline */
	if ((attr & WSATTR_UNDERLINE) != 0) {
		DELTA(rp, -(ri->ri_stride << 1), COPY_UNIT *);
		*rp = fg;
		if (ri->ri_hwbits) {
			DELTA(hrp, -(ri->ri_stride << 1), COPY_UNIT *);
			*hrp = fg;
		}
	}
}

#undef	PUTCHAR_WIDTH1
#undef	PUTCHAR_WIDTH

#undef	COPY_UNIT
#undef	GET_GLYPH
