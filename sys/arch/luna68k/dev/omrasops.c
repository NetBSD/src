/* $NetBSD: omrasops.c,v 1.19.18.1 2018/06/25 07:25:43 pgoyette Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: omrasops.c,v 1.19.18.1 2018/06/25 07:25:43 pgoyette Exp $");

/*
 * Designed speficically for 'm68k bitorder';
 *	- most significant byte is stored at lower address,
 *	- most significant bit is displayed at left most on screen.
 * Implementation relies on;
 *	- first column is at 32bit aligned address,
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
static void	om1_cursor(void *, int, int, int);
static void	om4_cursor(void *, int, int, int);
static int	om_mapchar(void *, int, unsigned int *);
static void	om1_putchar(void *, int, int, u_int, long);
static void	om4_putchar(void *, int, int, u_int, long);
static void	om1_copycols(void *, int, int, int, int);
static void	om4_copycols(void *, int, int, int, int);
static void	om1_copyrows(void *, int, int, int num);
static void	om4_copyrows(void *, int, int, int num);
static void	om1_erasecols(void *, int, int, int, long);
static void	om4_erasecols(void *, int, int, int, long);
static void	om1_eraserows(void *, int, int, long);
static void	om4_eraserows(void *, int, int, long);
static int	om1_allocattr(void *, int, int, int, long *);
static int	om4_allocattr(void *, int, int, int, long *);
static void	om4_unpack_attr(long, int *, int *, int *);

static int	omrasops_init(struct rasops_info *, int, int);

#define	ALL1BITS	(~0U)
#define	ALL0BITS	(0U)
#define	BLITWIDTH	(32)
#define	ALIGNMASK	(0x1f)
#define	BYTESDONE	(4)

/*
 * macros to handle unaligned bit copy ops.
 * See src/sys/dev/rasops/rasops_mask.h for MI version.
 * Also refer src/sys/arch/hp300/dev/maskbits.h.
 * (which was implemented for ancient src/sys/arch/hp300/dev/grf_hy.c)
 */

/* luna68k version GETBITS() that gets w bits from bit x at psrc memory */
#define	FASTGETBITS(psrc, x, w, dst)					\
	asm("bfextu %3{%1:%2},%0"					\
	    : "=d" (dst) 						\
	    : "di" (x), "di" (w), "o" (*(uint32_t *)(psrc)))

/* luna68k version PUTBITS() that puts w bits from bit x at pdst memory */
/* XXX this macro assumes (x + w) <= 32 to handle unaligned residual bits */
#define	FASTPUTBITS(src, x, w, pdst)					\
	asm("bfins %3,%0{%1:%2}"					\
	    : "+o" (*(uint32_t *)(pdst))				\
	    : "di" (x), "di" (w), "d" (src)				\
	    : "memory" );

#define	GETBITS(psrc, x, w, dst)	FASTGETBITS(psrc, x, w, dst)
#define	PUTBITS(src, x, w, pdst)	FASTPUTBITS(src, x, w, pdst)

/*
 * Blit a character at the specified co-ordinates.
 */
static void
om1_putchar(void *cookie, int row, int startcol, u_int uc, long attr)
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
	inverse = ((attr & 0x00000001) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		/* set lmask as ROP mask value, with THROUGH mode */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = lmask;

		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			glyph = (glyph >> align) ^ inverse;

			*W(p) = glyph;

			p += scanspan;
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	} else {
		uint8_t *q = p;
		uint32_t lhalf, rhalf;

		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			lhalf = (glyph >> align) ^ inverse;
			/* set lmask as ROP mask value, with THROUGH mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] =
			    lmask;
			
			*W(p) = lhalf;

			p += BYTESDONE;

			rhalf = (glyph << (BLITWIDTH - align)) ^ inverse;
			/* set rmask as ROP mask value, with THROUGH mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] =
			    rmask;

			*W(p) = rhalf;

			p = (q += scanspan);
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	}
}

static void
om4_putchar(void *cookie, int row, int startcol, u_int uc, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask, glyph, glyphbg, fgpat, bgpat;
	uint32_t fgmask0, fgmask1, fgmask2, fgmask3;
	uint32_t bgmask0, bgmask1, bgmask2, bgmask3;
	int i, fg, bg;
	uint8_t *fb;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	fb = (uint8_t *)ri->ri_font->data +
	    (uc - ri->ri_font->firstchar) * ri->ri_fontscale;
	om4_unpack_attr(attr, &fg, &bg, NULL);
	fgmask0 = (fg & 0x01) ? ALL1BITS : ALL0BITS;
	fgmask1 = (fg & 0x02) ? ALL1BITS : ALL0BITS;
	fgmask2 = (fg & 0x04) ? ALL1BITS : ALL0BITS;
	fgmask3 = (fg & 0x08) ? ALL1BITS : ALL0BITS;
	bgmask0 = (bg & 0x01) ? ALL1BITS : ALL0BITS;
	bgmask1 = (bg & 0x02) ? ALL1BITS : ALL0BITS;
	bgmask2 = (bg & 0x04) ? ALL1BITS : ALL0BITS;
	bgmask3 = (bg & 0x08) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = ri->ri_font->fontwidth + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);

	/* select all planes for later ROP function target */
	*(volatile uint32_t *)OMFB_PLANEMASK = 0xff;

	if (width <= BLITWIDTH) {
		lmask &= rmask;
		/* set lmask as ROP mask value, with THROUGH mode */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = lmask;

		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			glyph = (glyph >> align);
			glyphbg = glyph ^ ALL1BITS;

			fgpat = glyph   & fgmask0;
			bgpat = glyphbg & bgmask0;
			*P0(p) = (fgpat | bgpat);
			fgpat = glyph   & fgmask1;
			bgpat = glyphbg & bgmask1;
			*P1(p) = (fgpat | bgpat);
			fgpat = glyph   & fgmask2;
			bgpat = glyphbg & bgmask2;
			*P2(p) = (fgpat | bgpat);
			fgpat = glyph   & fgmask3;
			bgpat = glyphbg & bgmask3;
			*P3(p) = (fgpat | bgpat);

			p += scanspan;
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	} else {
		uint8_t *q = p;
		uint32_t lhalf, rhalf;
		uint32_t lhalfbg, rhalfbg;

		while (height > 0) {
			glyph = 0;
			for (i = ri->ri_font->stride; i != 0; i--)
				glyph = (glyph << 8) | *fb++;
			glyph <<= (4 - ri->ri_font->stride) * NBBY;
			lhalf = (glyph >> align);
			lhalfbg = lhalf ^ ALL1BITS;
			/* set lmask as ROP mask value, with THROUGH mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] =
			    lmask;

			fgpat = lhalf   & fgmask0;
			bgpat = lhalfbg & bgmask0;
			*P0(p) = (fgpat | bgpat);
			fgpat = lhalf   & fgmask1;
			bgpat = lhalfbg & bgmask1;
			*P1(p) = (fgpat | bgpat);
			fgpat = lhalf   & fgmask2;
			bgpat = lhalfbg & bgmask2;
			*P2(p) = (fgpat | bgpat);
			fgpat = lhalf   & fgmask3;
			bgpat = lhalfbg & bgmask3;
			*P3(p) = (fgpat | bgpat);

			p += BYTESDONE;

			rhalf = (glyph << (BLITWIDTH - align));
			rhalfbg = rhalf ^ ALL1BITS;
			/* set rmask as ROP mask value, with THROUGH mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] =
			    rmask;

			fgpat = rhalf   & fgmask0;
			bgpat = rhalfbg & bgmask0;
			*P0(p) = (fgpat | bgpat);
			fgpat = rhalf   & fgmask1;
			bgpat = rhalfbg & bgmask1;
			*P1(p) = (fgpat | bgpat);
			fgpat = rhalf   & fgmask2;
			bgpat = rhalfbg & bgmask2;
			*P2(p) = (fgpat | bgpat);
			fgpat = rhalf   & fgmask3;
			bgpat = rhalfbg & bgmask3;
			*P3(p) = (fgpat | bgpat);

			p = (q += scanspan);
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	}
	/* select plane #0 only; XXX need this ? */
	*(volatile uint32_t *)OMFB_PLANEMASK = 0x01;
}

static void
om1_erasecols(void *cookie, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, w, y;
	uint32_t lmask, rmask, fill;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	fill = ((attr & 0x00000001) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = w + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		fill  &= lmask;
		while (height > 0) {
			*P0(p) = (*P0(p) & ~lmask) | fill;
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;
		while (height > 0) {
			*P0(p) = (*P0(p) & ~lmask) | (fill & lmask);
			width -= 2 * BLITWIDTH;
			while (width > 0) {
				p += BYTESDONE;
				*P0(p) = fill;
				width -= BLITWIDTH;
			}
			p += BYTESDONE;
			*P0(p) = (fill & rmask) | (*P0(p) & ~rmask);

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
}

static void
om4_erasecols(void *cookie, int row, int startcol, int ncols, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, w, y, fg, bg;
	uint32_t lmask, rmask, fill0, fill1, fill2, fill3;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * row;
	startx = ri->ri_font->fontwidth * startcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	om4_unpack_attr(attr, &fg, &bg, NULL);
	fill0 = ((bg & 0x01) != 0) ? ALL1BITS : ALL0BITS;
	fill1 = ((bg & 0x02) != 0) ? ALL1BITS : ALL0BITS;
	fill2 = ((bg & 0x04) != 0) ? ALL1BITS : ALL0BITS;
	fill3 = ((bg & 0x08) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + y * scanspan + ((startx / 32) * 4);
	align = startx & ALIGNMASK;
	width = w + align;
	lmask = ALL1BITS >> align;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	if (width <= BLITWIDTH) {
		lmask &= rmask;
		fill0 &= lmask;
		fill1 &= lmask;
		fill2 &= lmask;
		fill3 &= lmask;
		while (height > 0) {
			*P0(p) = (*P0(p) & ~lmask) | fill0;
			*P1(p) = (*P1(p) & ~lmask) | fill1;
			*P2(p) = (*P2(p) & ~lmask) | fill2;
			*P3(p) = (*P3(p) & ~lmask) | fill3;
			p += scanspan;
			height--;
		}
	} else {
		uint8_t *q = p;
		while (height > 0) {
			*P0(p) = (*P0(p) & ~lmask) | (fill0 & lmask);
			*P1(p) = (*P1(p) & ~lmask) | (fill1 & lmask);
			*P2(p) = (*P2(p) & ~lmask) | (fill2 & lmask);
			*P3(p) = (*P3(p) & ~lmask) | (fill3 & lmask);
			width -= 2 * BLITWIDTH;
			while (width > 0) {
				p += BYTESDONE;
				*P0(p) = fill0;
				*P1(p) = fill1;
				*P2(p) = fill2;
				*P3(p) = fill3;
				width -= BLITWIDTH;
			}
			p += BYTESDONE;
			*P0(p) = (fill0 & rmask) | (*P0(p) & ~rmask);
			*P1(p) = (fill1 & rmask) | (*P1(p) & ~rmask);
			*P2(p) = (fill2 & rmask) | (*P2(p) & ~rmask);
			*P3(p) = (fill3 & rmask) | (*P3(p) & ~rmask);

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
}

static void
om1_eraserows(void *cookie, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p, *q;
	int scanspan, starty, height, width, w;
	uint32_t rmask, fill;

	scanspan = ri->ri_stride;
	starty = ri->ri_font->fontheight * startrow;
	height = ri->ri_font->fontheight * nrows;
	w = ri->ri_emuwidth;
	fill = ((attr & 0x00000001) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + starty * scanspan;
	width = w;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	q = p;
	while (height > 0) {
		*P0(p) = fill;				/* always aligned */
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			*P0(p) = fill;
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		*P0(p) = (fill & rmask) | (*P0(p) & ~rmask);
		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om4_eraserows(void *cookie, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = cookie;
	uint8_t *p, *q;
	int scanspan, starty, height, width, w, fg, bg;
	uint32_t rmask, fill0, fill1, fill2, fill3;

	scanspan = ri->ri_stride;
	starty = ri->ri_font->fontheight * startrow;
	height = ri->ri_font->fontheight * nrows;
	w = ri->ri_emuwidth;
	om4_unpack_attr(attr, &fg, &bg, NULL);
	fill0 = ((bg & 0x01) != 0) ? ALL1BITS : ALL0BITS;
	fill1 = ((bg & 0x02) != 0) ? ALL1BITS : ALL0BITS;
	fill2 = ((bg & 0x04) != 0) ? ALL1BITS : ALL0BITS;
	fill3 = ((bg & 0x08) != 0) ? ALL1BITS : ALL0BITS;

	p = (uint8_t *)ri->ri_bits + starty * scanspan;
	width = w;
	rmask = ALL1BITS << (-width & ALIGNMASK);
	q = p;
	while (height > 0) {
		*P0(p) = fill0;				/* always aligned */
		*P1(p) = fill1;
		*P2(p) = fill2;
		*P3(p) = fill3;
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			*P0(p) = fill0;
			*P1(p) = fill1;
			*P2(p) = fill2;
			*P3(p) = fill3;
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		*P0(p) = (fill0 & rmask) | (*P0(p) & ~rmask);
		*P1(p) = (fill1 & rmask) | (*P1(p) & ~rmask);
		*P2(p) = (fill2 & rmask) | (*P2(p) & ~rmask);
		*P3(p) = (fill3 & rmask) | (*P3(p) & ~rmask);
		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om1_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
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
		*P0(p + offset) = *P0(p);		/* always aligned */
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			*P0(p + offset) = *P0(p);
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		*P0(p + offset) = (*P0(p) & rmask) | (*P0(p + offset) & ~rmask);

		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om4_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
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
		*P0(p + offset) = *P0(p);		/* always aligned */
		*P1(p + offset) = *P1(p);
		*P2(p + offset) = *P2(p);
		*P3(p + offset) = *P3(p);
		width -= 2 * BLITWIDTH;
		while (width > 0) {
			p += BYTESDONE;
			*P0(p + offset) = *P0(p);
			*P1(p + offset) = *P1(p);
			*P2(p + offset) = *P2(p);
			*P3(p + offset) = *P3(p);
			width -= BLITWIDTH;
		}
		p += BYTESDONE;
		*P0(p + offset) = (*P0(p) & rmask) | (*P0(p + offset) & ~rmask);
		*P1(p + offset) = (*P1(p) & rmask) | (*P1(p + offset) & ~rmask);
		*P2(p + offset) = (*P2(p) & rmask) | (*P2(p + offset) & ~rmask);
		*P3(p + offset) = (*P3(p) & rmask) | (*P3(p + offset) & ~rmask);

		p = (q += scanspan);
		width = w;
		height--;
	}
}

static void
om1_copycols(void *cookie, int startrow, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	uint8_t *sp, *dp, *sq, *dq, *basep;
	int scanspan, height, w, y, srcx, dstx;
	int sb, eb, db, sboff, full, cnt, lnum, rnum;
	uint32_t lmask, rmask, tmp;
	bool sbover;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	srcx = ri->ri_font->fontwidth * srccol;
	dstx = ri->ri_font->fontwidth * dstcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	basep = (uint8_t *)ri->ri_bits + y * scanspan;

	sb = srcx & ALIGNMASK;
	db = dstx & ALIGNMASK;

	if (db + w <= BLITWIDTH) {
		/* Destination is contained within a single word */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		while (height > 0) {
			GETBITS(P0(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P0(dp));
			dp += scanspan;
			sp += scanspan;
			height--;
		}
		return;
	}

	lmask = (db == 0) ? 0 : ALL1BITS >> db;
	eb = (db + w) & ALIGNMASK;
	rmask = (eb == 0) ? 0 : ALL1BITS << (32 - eb); 
	lnum = (32 - db) & ALIGNMASK;
	rnum = (dstx + w) & ALIGNMASK;

	if (lmask != 0)
		full = (w - (32 - db)) / 32;
	else
		full = w / 32;

	sbover = (sb + lnum) >= 32;

	if (dstcol < srccol || srccol + ncols < dstcol) {
		/* copy forward (left-to-right) */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		if (lmask != 0) {
			sboff = sb + lnum;
			if (sboff >= 32)
				sboff -= 32;
		} else
			sboff = sb;

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (lmask != 0) {
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				dp += BYTESDONE;
				if (sbover)
					sp += BYTESDONE;
			}

			for (cnt = full; cnt; cnt--) {
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				sp += BYTESDONE;
				dp += BYTESDONE;
			}

			if (rmask != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	} else {
		/* copy backward (right-to-left) */
		sp = basep + ((srcx + w) / 32) * 4;
		dp = basep + ((dstx + w) / 32) * 4;

		sboff = (srcx + w) & ALIGNMASK;
		sboff -= rnum;
		if (sboff < 0) {
			sp -= BYTESDONE;
			sboff += 32;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (rnum != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
			}

			for (cnt = full; cnt; cnt--) {
				sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
			}

			if (lmask != 0) {
				if (sbover)
					sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	}
}

static void
om4_copycols(void *cookie, int startrow, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	uint8_t *sp, *dp, *sq, *dq, *basep;
	int scanspan, height, w, y, srcx, dstx;
	int sb, eb, db, sboff, full, cnt, lnum, rnum;
	uint32_t lmask, rmask, tmp;
	bool sbover;

	scanspan = ri->ri_stride;
	y = ri->ri_font->fontheight * startrow;
	srcx = ri->ri_font->fontwidth * srccol;
	dstx = ri->ri_font->fontwidth * dstcol;
	height = ri->ri_font->fontheight;
	w = ri->ri_font->fontwidth * ncols;
	basep = (uint8_t *)ri->ri_bits + y * scanspan;

	sb = srcx & ALIGNMASK;
	db = dstx & ALIGNMASK;

	if (db + w <= BLITWIDTH) {
		/* Destination is contained within a single word */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		while (height > 0) {
			GETBITS(P0(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P0(dp));
			GETBITS(P1(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P1(dp));
			GETBITS(P2(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P2(dp));
			GETBITS(P3(sp), sb, w, tmp);
			PUTBITS(tmp, db, w, P3(dp));
			dp += scanspan;
			sp += scanspan;
			height--;
		}
		return;
	}

	lmask = (db == 0) ? 0 : ALL1BITS >> db;
	eb = (db + w) & ALIGNMASK;
	rmask = (eb == 0) ? 0 : ALL1BITS << (32 - eb); 
	lnum = (32 - db) & ALIGNMASK;
	rnum = (dstx + w) & ALIGNMASK;

	if (lmask != 0)
		full = (w - (32 - db)) / 32;
	else
		full = w / 32;

	sbover = (sb + lnum) >= 32;

	if (dstcol < srccol || srccol + ncols < dstcol) {
		/* copy forward (left-to-right) */
		sp = basep + (srcx / 32) * 4;
		dp = basep + (dstx / 32) * 4;

		if (lmask != 0) {
			sboff = sb + lnum;
			if (sboff >= 32)
				sboff -= 32;
		} else
			sboff = sb;

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (lmask != 0) {
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				GETBITS(P1(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P1(dp));
				GETBITS(P2(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P2(dp));
				GETBITS(P3(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P3(dp));
				dp += BYTESDONE;
				if (sbover)
					sp += BYTESDONE;
			}

			for (cnt = full; cnt; cnt--) {
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				GETBITS(P1(sp), sboff, 32, tmp);
				*P1(dp) = tmp;
				GETBITS(P2(sp), sboff, 32, tmp);
				*P2(dp) = tmp;
				GETBITS(P3(sp), sboff, 32, tmp);
				*P3(dp) = tmp;
				sp += BYTESDONE;
				dp += BYTESDONE;
			}

			if (rmask != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
				GETBITS(P1(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P1(dp));
				GETBITS(P2(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P2(dp));
				GETBITS(P3(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P3(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	} else {
		/* copy backward (right-to-left) */
		sp = basep + ((srcx + w) / 32) * 4;
		dp = basep + ((dstx + w) / 32) * 4;

		sboff = (srcx + w) & ALIGNMASK;
		sboff -= rnum;
		if (sboff < 0) {
			sp -= BYTESDONE;
			sboff += 32;
		}

		sq = sp;
		dq = dp;
		while (height > 0) {
			if (rnum != 0) {
				GETBITS(P0(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P0(dp));
				GETBITS(P1(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P1(dp));
				GETBITS(P2(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P2(dp));
				GETBITS(P3(sp), sboff, rnum, tmp);
				PUTBITS(tmp, 0, rnum, P3(dp));
			}

			for (cnt = full; cnt; cnt--) {
				sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sboff, 32, tmp);
				*P0(dp) = tmp;
				GETBITS(P1(sp), sboff, 32, tmp);
				*P1(dp) = tmp;
				GETBITS(P2(sp), sboff, 32, tmp);
				*P2(dp) = tmp;
				GETBITS(P3(sp), sboff, 32, tmp);
				*P3(dp) = tmp;
			}

			if (lmask != 0) {
				if (sbover)
					sp -= BYTESDONE;
				dp -= BYTESDONE;
				GETBITS(P0(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P0(dp));
				GETBITS(P1(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P1(dp));
				GETBITS(P2(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P2(dp));
				GETBITS(P3(sp), sb, lnum, tmp);
				PUTBITS(tmp, db, lnum, P3(dp));
			}

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			height--;
		}
	}
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
om1_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask;

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
		/* set lmask as ROP mask value, with INV2 mode */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = lmask;

		while (height > 0) {
			*P0(p) = ALL1BITS;
			p += scanspan;
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	} else {
		uint8_t *q = p;

		while (height > 0) {
			/* set lmask as ROP mask value, with INV2 mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = lmask;
			*W(p) = ALL1BITS;

			p += BYTESDONE;

			/* set lmask as ROP mask value, with INV2 mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = rmask;
			*W(p) = ALL1BITS;

			p = (q += scanspan);
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	}
	ri->ri_flg ^= RI_CURSOR;
}

static void
om4_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	uint8_t *p;
	int scanspan, startx, height, width, align, y;
	uint32_t lmask, rmask;

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

	/* select all planes for later ROP function target */
	*(volatile uint32_t *)OMFB_PLANEMASK = 0xff;

	if (width <= BLITWIDTH) {
		lmask &= rmask;
		/* set lmask as ROP mask value, with INV2 mode */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = lmask;

		while (height > 0) {
			*W(p) = ALL1BITS;
			p += scanspan;
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	} else {
		uint8_t *q = p;

		while (height > 0) {
			/* set lmask as ROP mask value, with INV2 mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = lmask;
			*W(p) = ALL1BITS;

			p += BYTESDONE;

			/* set rmask as ROP mask value, with INV2 mode */
			((volatile uint32_t *)OMFB_ROPFUNC)[ROP_INV2] = rmask;
			*W(p) = ALL1BITS;

			p = (q += scanspan);
			height--;
		}
		/* reset mask value */
		((volatile uint32_t *)OMFB_ROPFUNC)[ROP_THROUGH] = ALL1BITS;
	}
	/* select plane #0 only; XXX need this ? */
	*(volatile uint32_t *)OMFB_PLANEMASK = 0x01;

	ri->ri_flg ^= RI_CURSOR;
}

/*
 * Allocate attribute. We just pack these into an integer.
 */
static int
om1_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{

	if ((flags & (WSATTR_HILIT | WSATTR_BLINK |
	    WSATTR_UNDERLINE | WSATTR_WSCOLORS)) != 0)
		return EINVAL;
	if ((flags & WSATTR_REVERSE) != 0)
		*attrp = 1;
	else
		*attrp = 0;
	return 0;
}

static int
om4_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{

	if ((flags & (WSATTR_BLINK | WSATTR_UNDERLINE)) != 0)
		return EINVAL;
	if ((flags & WSATTR_WSCOLORS) == 0) {
		fg = WSCOL_WHITE;
		bg = WSCOL_BLACK;
	}

	if ((flags & WSATTR_REVERSE) != 0) {
		int swap;
		swap = fg;
		fg = bg;
		bg = swap;
	}

	if ((flags & WSATTR_HILIT) != 0)
		fg += 8;

	*attrp = (fg << 24) | (bg << 16);
	return 0;
}

static void
om4_unpack_attr(long attr, int *fg, int *bg, int *underline)
{

	*fg = ((u_int)attr >> 24) & 0xf;
	*bg = ((u_int)attr >> 16) & 0xf;
}

/*
 * Init subset of rasops(9) for omrasops.
 */
int
omrasops1_init(struct rasops_info *ri, int wantrows, int wantcols)
{

	omrasops_init(ri, wantrows, wantcols);

	/* fill our own emulops */
	ri->ri_ops.cursor    = om1_cursor;
	ri->ri_ops.mapchar   = om_mapchar;
	ri->ri_ops.putchar   = om1_putchar;
	ri->ri_ops.copycols  = om1_copycols;
	ri->ri_ops.erasecols = om1_erasecols;
	ri->ri_ops.copyrows  = om1_copyrows;
	ri->ri_ops.eraserows = om1_eraserows;
	ri->ri_ops.allocattr = om1_allocattr;
	ri->ri_caps = WSSCREEN_REVERSE;

	ri->ri_flg |= RI_CFGDONE;

	return 0;
}

int
omrasops4_init(struct rasops_info *ri, int wantrows, int wantcols)
{

	omrasops_init(ri, wantrows, wantcols);

	/* fill our own emulops */
	ri->ri_ops.cursor    = om4_cursor;
	ri->ri_ops.mapchar   = om_mapchar;
	ri->ri_ops.putchar   = om4_putchar;
	ri->ri_ops.copycols  = om4_copycols;
	ri->ri_ops.erasecols = om4_erasecols;
	ri->ri_ops.copyrows  = om4_copyrows;
	ri->ri_ops.eraserows = om4_eraserows;
	ri->ri_ops.allocattr = om4_allocattr;
	ri->ri_caps = WSSCREEN_HILIT | WSSCREEN_WSCOLORS | WSSCREEN_REVERSE;

	ri->ri_flg |= RI_CFGDONE;

	return 0;
}

static int
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

	/* all planes are independently addressed */
	bpp = 1;

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

	return 0;
}
