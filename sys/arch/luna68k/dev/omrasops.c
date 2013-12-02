/* $NetBSD: omrasops.c,v 1.14 2013/12/02 13:45:40 tsutsui Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: omrasops.c,v 1.14 2013/12/02 13:45:40 tsutsui Exp $");

/*
 * Designed speficically for 'm68k bitorder';
 *	- most significant byte is stored at lower address,
 *	- most significant bit is displayed at left most on screen.
 * Implementation relies on;
 *	- every memory references is done in aligned 32bit chunk,
 *	- font glyphs are stored in 32bit padded.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <arch/luna68k/dev/omrasopsvar.h>

/* wscons emulator operations */
static void	om_cursor(void *, int, int, int);
static int	om_mapchar(void *, int, unsigned int *);
static void	om_putchar(void *, int, int, u_int, long);
static void	om_copycols(void *, int, int, int, int);
static void	om_copyrows(void *, int, int, int num);
static void	om_erasecols(void *, int, int, int, long);
static void	om_eraserows(void *, int, int, long);
static int	om_allocattr(void *, int, int, int, long *);

#define	ALL1BITS	(~0U)
#define	ALL0BITS	(0U)
#define	BLITWIDTH	(32)
#define	ALIGNMASK	(0x1f)
#define	BYTESDONE	(4)

#define	W(p) (*(uint32_t *)(p))
#define	R(p) (*(uint32_t *)((uint8_t *)(p) + 0x40000))

/*
 * Blit a character at the specified co-ordinates.
 */
static void
om_putchar(void *cookie, int row, int startcol, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask, glyph, inverse;
	int i;
	uint8_t *fb;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	fb = (uint8_t *)ri->ri_font->data +
	    (uc - ri->ri_font->firstchar) * ri->ri_fontscale;
	inverse = (attr != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			glyph = (glyph >> align) ^ inverse;
			W(p) = (R(p) & ~lmask) | (glyph & lmask);
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;
		uint32_t lhalf, rhalf;

		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			lhalf = (glyph >> align) ^ inverse;
			W(p) = (R(p) & ~lmask) | (lhalf & lmask);
			p += BYTESDONE;
			rhalf = (glyph << (BLITWIDTH - align)) ^ inverse;
			W(p) = (rhalf & rmask) | (R(p) & ~rmask);

			p = (q += scanspan);
			height--;
		}
	}
}

static void
om_erasecols(void *cookie, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, w, y;
	uint32_t lmask, rmask, fill;

	scanspan = ri->ri_stride;;
	fill = (attr != 0) ? ALL1BITS : ALL0BITS;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	fill = (attr != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = w + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		fill &= lmask;
		while (height > 0) {
			W(p) = (R(p) & ~lmask) | fill;
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;
		while (height > 0) {
			W(p) = (R(p) & ~lmask) | (fill & lmask);
			width -= 2 * BLITWIDTH;
			while (width > 0) {
				p += BYTESDONE;
				W(p) = fill;
				width -= BLITWIDTH;
			}
			p += BYTESDONE;
			W(p) = (fill & rmask) | (R(p) & ~rmask);

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
}

static void
om_eraserows(void *cookie, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p, *q;
	int scanspan, starty, height, width, w;
	uint32_t rmask, fill;

	scanspan = ri->ri_stride;
	starty = ri->ri_font->fontheight * startrow;
	height = ri->ri_font->fontheight * nrows;
	w = ri->ri_emuwidth;
	fill = (attr != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + starty * scanspan;
	width = w;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	q = p;
	while (height > 0) {
		W(p) = fill;				/* always aligned */
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			W(p) = fill;
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		W(p) = (fill & rmask) | (R(p) & ~rmask);
		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	uint8_t *p, *q;
	int scanspan, offset, srcy, height, width, w;
	uint32_t rmask;

	scanspan = ri->ri_stride;
	height = ri->ri_font->fontheight * nrows;
	offset = (dstrow - srcrow) * scanspan * ri->ri_font->fontheight;
	srcy = ri->ri_font->fontheight * srcrow;
	if (srcrow < dstrow && srcrow + nrows > dstrow) {
		scanspan = -scanspan;
		srcy = srcy + height - 1;
	}

	p = (uint8_t *)ri->ri_bits + srcy * ri->ri_stride;
	w = ri->ri_emuwidth;
	width = w;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	q = p;
	while (height > 0) {
		W(p + offset) = R(p);			/* always aligned */
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			W(p + offset) = R(p);
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		W(p + offset) = (R(p) & rmask) | (R(p + offset) & ~rmask);

		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om_copycols(void *cookie, int startrow, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	uint8_t *sp, *dp, *basep;
	int scanspan, height, width, align, shift, w, y, srcx, dstx;
	uint32_t lmask, rmask;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	srcx = ri->ri_font->fontwidth * srccol;
	dstx = ri->ri_font->fontwidth * dstcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	basep = (uint8_t *)ri->ri_bits + y * scanspan;

	align = shift = srcx & ALIGNMASK;
	width = w + align;
	align = dstx & ALIGNMASK;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-(w + align) & ALIGNMASK);
	shift = align - shift;
	sp = basep + (srcx / 32) * 4;
	dp = basep + (dstx / 32) * 4;

	if (shift != 0)
		goto hardluckalignment;

	/* alignments comfortably match */
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		while (height > 0) {
			W(dp) = (R(dp) & ~lmask) | (R(sp) & lmask);
			dp += scanspan;
			sp += scanspan;
			height--;
		}
	}
	/* copy forward (left-to-right) */
	else if (dstcol < srccol || srccol + ncols < dstcol) {
		uint8_t *sq = sp, *dq = dp;

		w = width;
		while (height > 0) {
			W(dp) = (R(dp) & ~lmask) | (R(sp) & lmask);
			width -= 2 * BLITWIDTH;
			while (width > 0) {
				sp += BYTESDONE;
				dp += BYTESDONE;
				W(dp) = R(sp);
				width -= BLITWIDTH;
			}
			sp += BYTESDONE;
			dp += BYTESDONE;
			W(dp) = (R(sp) & rmask) | (R(dp) & ~rmask);
			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	/* copy backward (right-to-left) */
	else {
		uint8_t *sq, *dq;

		sq = (sp += width / 32 * 4);
		dq = (dp += width / 32 * 4);
		w = width;
		while (height > 0) {
			W(dp) = (R(sp) & rmask) | (R(dp) & ~rmask);
			width -= 2 * BLITWIDTH;
			while (width > 0) {
				sp -= BYTESDONE;
				dp -= BYTESDONE;
				W(dp) = R(sp);
				width -= BLITWIDTH;
			}
			sp -= BYTESDONE;
			dp -= BYTESDONE;
			W(dp) = (R(dp) & ~lmask) | (R(sp) & lmask);

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	return;

    hardluckalignment:
	/* alignments painfully disagree */
	return;
}

/*
 * Map a character.
 */
static int
om_mapchar(void *cookie, int c, u_int *cp)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *wf = ri->ri_font;

	if (wf->encoding != WSDISPLAY_FONTENC_ISO) {
		c = wsfont_map_unichar(wf, c);

		if (c < 0)
			goto fail;
	}
	if (c < wf->firstchar || c >= (wf->firstchar + wf->numchars))
		goto fail;

	*cp = c;
	return 5;

 fail:
	*cp = ' ';
	return 0;
}

/*
 * Position|{enable|disable} the cursor at the specified location.
 */
static void
om_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask, image;

	if (!on) {
		/* make sure it's on */
		if ((ri->ri_flg & RI_CURSOR) == 0)
			return;

		row = ri->ri_crow;
		col = ri->ri_ccol;
	} else {
		/* unpaint the old copy. */
		ri->ri_crow = row;
		ri->ri_ccol = col;
	}

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * col;
	height = ri->ri_font->fontheight;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		while (height > 0) {
			image = R(p);
			W(p) = (image & ~lmask) | ((image ^ ALL1BITS) & lmask);
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;

		while (height > 0) {
			image = R(p);
			W(p) = (image & ~lmask) | ((image ^ ALL1BITS) & lmask);
			p += BYTESDONE;
			image = R(p);
			W(p) = ((image ^ ALL1BITS) & rmask) | (image & ~rmask);

			p = (q += scanspan);
			height--;
		}
	}
	ri->ri_flg ^= RI_CURSOR;
}

/*
 * Allocate attribute. We just pack these into an integer.
 */
static int
om_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{

	if (flags & (WSATTR_HILIT | WSATTR_BLINK |
	    WSATTR_UNDERLINE | WSATTR_WSCOLORS))
		return EINVAL;
	if (flags & WSATTR_REVERSE)
		*attrp = 1;
	else
		*attrp = 0;
	return 0;
}

/*
 * Init subset of rasops(9) for omrasops.
 */
int
omrasops_init(struct rasops_info *ri, int wantrows, int wantcols)
{
	int wsfcookie, bpp;

	if (wantrows == 0)
		wantrows = 34;
	if (wantrows < 10)
		wantrows = 10;
	if (wantcols == 0)
		wantcols = 80;
	if (wantcols < 20)
		wantcols = 20;

	/* Use default font */
	wsfont_init();
	wsfcookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (wsfcookie < 0)
		panic("%s: no font available", __func__);
	if (wsfont_lock(wsfcookie, &ri->ri_font))
		panic("%s: unable to lock font", __func__);
	ri->ri_wsfcookie = wsfcookie;

	KASSERT(ri->ri_font->fontwidth > 4 && ri->ri_font->fontwidth <= 32);

	bpp = ri->ri_depth;

	/* Now constrain what they get */
	ri->ri_emuwidth = ri->ri_font->fontwidth * wantcols;
	ri->ri_emuheight = ri->ri_font->fontheight * wantrows;
	if (ri->ri_emuwidth > ri->ri_width)
		ri->ri_emuwidth = ri->ri_width;
	if (ri->ri_emuheight > ri->ri_height)
		ri->ri_emuheight = ri->ri_height;

	/* Reduce width until aligned on a 32-bit boundary */
	while ((ri->ri_emuwidth * bpp & 31) != 0)
		ri->ri_emuwidth--;

	ri->ri_cols = ri->ri_emuwidth / ri->ri_font->fontwidth;
	ri->ri_rows = ri->ri_emuheight / ri->ri_font->fontheight;
	ri->ri_emustride = ri->ri_emuwidth * bpp >> 3;
	ri->ri_delta = ri->ri_stride - ri->ri_emustride;
	ri->ri_ccol = 0;
	ri->ri_crow = 0;
	ri->ri_pelbytes = bpp >> 3;

	ri->ri_xscale = (ri->ri_font->fontwidth * bpp) >> 3;
	ri->ri_yscale = ri->ri_font->fontheight * ri->ri_stride;
	ri->ri_fontscale = ri->ri_font->fontheight * ri->ri_font->stride;

	/* Clear the entire display */
	if ((ri->ri_flg & RI_CLEAR) != 0)
		memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height);

	/* Now centre our window if needs be */
	ri->ri_origbits = ri->ri_bits;

	if ((ri->ri_flg & RI_CENTER) != 0) {
		ri->ri_bits += (((ri->ri_width * bpp >> 3) -
		    ri->ri_emustride) >> 1) & ~3;
		ri->ri_bits += ((ri->ri_height - ri->ri_emuheight) >> 1) *
		    ri->ri_stride;
		ri->ri_yorigin = (int)(ri->ri_bits - ri->ri_origbits)
		   / ri->ri_stride;
		ri->ri_xorigin = (((int)(ri->ri_bits - ri->ri_origbits)
		   % ri->ri_stride) * 8 / bpp);
	} else
		ri->ri_xorigin = ri->ri_yorigin = 0;

	/* fill our own emulops */
	ri->ri_ops.cursor    = om_cursor;
	ri->ri_ops.mapchar   = om_mapchar;
	ri->ri_ops.putchar   = om_putchar;
	ri->ri_ops.copycols  = om_copycols;
	ri->ri_ops.erasecols = om_erasecols;
	ri->ri_ops.copyrows  = om_copyrows;
	ri->ri_ops.eraserows = om_eraserows;
	ri->ri_ops.allocattr = om_allocattr;
	ri->ri_caps = WSSCREEN_REVERSE;

	ri->ri_flg |= RI_CFGDONE;

	return 0;
}
