/* $NetBSD: sfbtextops.i,v 1.1.2.1 1999/12/27 18:35:39 wrstuden Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is a part of sfbptextops.c/sfbptextops32.c and to be
 * included by them on compilation.
 *
 * One #define symbol controls SFB+ features.
 *
 * SFBBPP
 *	value either 8 or 32.
 *
 * XXX Pursue the differences between SFB and SFB+, and merge sfb.c. XXX
 */

#ifndef SFBBPP
#define	SFBBPP 8
#endif

#define	MODE_SIMPLE		0
#define	MODE_OPAQUESTIPPLE	1
#define	MODE_OPAQUELINE		2
#define	MODE_TRANSPARENTSTIPPLE	5
#define	MODE_TRANSPARENTLINE	6
#define	MODE_COPY		7

#define	MODE_TRANSPARENTFILL	(5+0x20)

#if SFBBPP == 8
/* parameters for 8bpp configuration */
#define	SFBALIGNMASK		0x7
#define	SFBPIXELBYTES		1
#define	SFBSTIPPLEALL1		0xffffffff
#define	SFBSTIPPLEBITS		32
#define	SFBSTIPPLEBITMASK	0x1f
#define	SFBSTIPPLEBYTESDONE	32
#define	SFBCOPYALL1		0xffffffff
#define	SFBCOPYBITS		32
#define	SFBCOPYBITMASK		0x1f
#define	SFBCOPYBYTESDONE	32
#define	XXXCOLOR		0x01010101

#elif SFBBPP == 32
/* parameters for 32bpp configuration */
#define	SFBALIGNMASK		0x7
#define	SFBPIXELBYTES		4
#define	SFBSTIPPLEALL1		0x0000ffff
#define	SFBSTIPPLEBITS		16
#define	SFBSTIPPLEBITMASK	0xf
#define	SFBSTIPPLEBYTESDONE	32
#define	SFBCOPYALL1		0x000000ff
#define	SFBCOPYBITS		8
#define	SFBCOPYBITMASK		0x3
#define	SFBCOPYBYTESDONE	32
#define	XXXCOLOR		0x00ffffff
#endif

#ifdef pmax
#define	WRITE_MB()
#endif

#ifdef alpha
#define	WRITE_MB() tc_wmb()
#endif

#define	SFBMODE(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_MODE) = (v))
#define	SFBROP(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_ROP) = (v))
#define	SFBPLANEMASK(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_PLANEMASK) = (v))
#define	SFBPIXELMASK(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_PIXELMASK) = (v))
#define	SFBADDRESS(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_ADDRESS) = (v))
#define	SFBSTART(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_START) = (v))
#define	SFBPIXELSHIFT(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_PIXELSHIFT) = (v))
#define	SFBFG(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_FG) = (v))
#define	SFBBG(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_BG) = (v))
#define	SFBBCONT(p, v) \
		(*(u_int32_t *)((p) + SFB_ASIC_BCONT) = (v))

#define	SFBDATA(p, v) \
		(*((u_int32_t *)(p) + TGA_REG_GDAR) = (v))

#if SFBBPP == 8
#define	TEXTOP(name) sfbp_ ## name
#elif SFBBPP == 32
#define	TEXTOP(name) sfbp_ ## name ## 32
#endif

static void TEXTOP(cursor) __P((void *, int, int, int));
static int  TEXTOP(mapchar) __P((void *, int, unsigned int *));
static void TEXTOP(putchar) __P((void *, int, int, u_int, long));
static void TEXTOP(copycols) __P((void *, int, int, int, int));
static void TEXTOP(erasecols) __P((void *, int, int, int, long));
static void TEXTOP(copyrows) __P((void *, int, int, int));
static void TEXTOP(eraserows) __P((void *, int, int, long));
extern int  sfb_alloc_attr __P((void *, int, int, int, long *));

const struct wsdisplay_emulops TEXTOP(emulops) = {
	TEXTOP(cursor),			/* could use hardware cursor; punt */
	TEXTOP(mapchar),
	TEXTOP(putchar),
	TEXTOP(copycols),
	TEXTOP(erasecols),
	TEXTOP(copyrows),
	TEXTOP(eraserows),
	sfb_alloc_attr
};

/*
 * Paint (or unpaint) the cursor.
 */
static void
TEXTOP(cursor)(id, on, row, col)
	void *id;
	int on, row, col;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, height, x, y, count;

	/* turn the cursor off */
	if (!on) {
		/* make sure it's on */
		if ((rc->rc_bits & RC_CURSOR) == 0)
			return;

		row = *rc->rc_crowp;
		col = *rc->rc_ccolp;
	} else {
		/* unpaint the old copy. */
		*rc->rc_crowp = row;
		*rc->rc_ccolp = col;
	}

	x = col * rc->rc_font->width + rc->rc_xorigin;
	y = row * rc->rc_font->height + rc->rc_yorigin;
	scanspan = rap->linelongs * 4;
	height = rc->rc_font->height;
	count = rc->rc_font->width - 1;

	p = (caddr_t)rap->pixels + y * scanspan + x * SFBPIXELBYTES;
	sfb = rap->data;

	SFBMODE(sfb, MODE_TRANSPARENTFILL);
	SFBPLANEMASK(sfb, ~0);
	SFBROP(sfb, 6);			/* ROP_XOR */
	SFBFG(sfb, XXXCOLOR);		/* (fg ^ bg) to swap fg/bg */
	SFBDATA(sfb, ~0);		/* fill mask */
	while (height > 0) {
		SFBADDRESS(sfb, (long)p);
		SFBBCONT(sfb, count);
		p += scanspan;
		height--;
	}
	SFBMODE(sfb, MODE_SIMPLE);
	SFBROP(sfb, 3);			/* ROP_COPY */

	rc->rc_bits ^= RC_CURSOR;
}

/*
 * Actually write a string to the frame buffer.
 */
static int
TEXTOP(mapchar)(id, uni, index)
	void *id;
	int uni;
	unsigned int *index;
{
	if (uni < 128) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

/*
 * Actually write a string to the frame buffer.
 */
static void
TEXTOP(putchar)(id, row, col, uc, attr)
	void *id;
	int row, col;
	u_int uc;
	long attr;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, height, width, align, x, y;
	u_int32_t lmask, rmask, glyph;
	u_int32_t *g;

if (uc < 0x20 || uc >= 127) return; /* XXX why \033 is creaping in !? XXX */

	x = col * rc->rc_font->width + rc->rc_xorigin;
	y = row * rc->rc_font->height + rc->rc_yorigin;
	scanspan = rap->linelongs * 4;
	height = rc->rc_font->height;
	g = rc->rc_font->chars[uc].r->pixels;

	p = (caddr_t)rap->pixels + y * scanspan + x * SFBPIXELBYTES;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	align /= SFBPIXELBYTES;
	width = rc->rc_font->width + align;
	lmask = SFBSTIPPLEALL1 << align;
	rmask = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;
	attr = (attr != 0) ^ (rc->rc_bits & RC_INVERT);

	SFBMODE(sfb, MODE_OPAQUESTIPPLE);
	SFBPLANEMASK(sfb, ~0);
	SFBFG(sfb, (attr == 0) ? XXXCOLOR : 0);
	SFBBG(sfb, (attr == 0) ? 0 : XXXCOLOR);
	if (width <= SFBSTIPPLEBITS) {
		lmask = lmask & rmask;
		while (height > 0) {
			SFBPIXELMASK(sfb, lmask);
WRITE_MB();
#if 0				/* not sure this can work */
			SFBADDRESS(sfb, (long)p);
			SFBBCONT(sfb, glyph << align);
#else				/* safer but slower */
			*(u_int32_t *)p = glyph << align;
#endif
WRITE_MB();
			p += scanspan;
			g += 1;
			height--;
		}
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			glyph = *g;
			SFBPIXELMASK(sfb, lmask);
WRITE_MB();
			*(u_int32_t *)p = glyph << align;
WRITE_MB();
			p += SFBSTIPPLEBYTESDONE;
			SFBPIXELMASK(sfb, rmask);
WRITE_MB();
			*(u_int32_t *)p = glyph >> (-width & SFBSTIPPLEBITMASK);
WRITE_MB();

			p = (q += scanspan);
			g += 1;
			height--;
		}
	}
	SFBMODE(sfb, MODE_SIMPLE);
	SFBPIXELMASK(sfb, ~0);		/* entire pixel */
}

/*
 * Copy characters in a line.
 */
static void
TEXTOP(copycols)(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sp, dp, basex, sfb;
	int scanspan, height, width, aligns, alignd, shift, w, y;
	u_int32_t lmasks, rmasks, lmaskd, rmaskd;

	scanspan = rap->linelongs * 4;
	y = rc->rc_yorigin + rc->rc_font->height * row;
	basex = (caddr_t)rap->pixels + y * scanspan + rc->rc_xorigin;
	height = rc->rc_font->height;
	w = rc->rc_font->width * ncols;

	sp = basex + rc->rc_font->width * srccol;
	aligns = (long)sp & SFBALIGNMASK;
	dp = basex + rc->rc_font->width * dstcol;
	alignd = (long)dp & SFBALIGNMASK;
	sfb = rap->data;

	SFBMODE(sfb, MODE_COPY);
	SFBPLANEMASK(sfb, ~0);

	/* copy forward (left-to-right) */
	if (dstcol < srccol || srccol + ncols < dstcol) {
		caddr_t sq, dq;

		shift = alignd - aligns;
		if (shift < 0) {
			dp -= 8;		/* prime left edge */
			alignd += 8;		/* compensate it */
			width = aligns + w + 8; /* adjust total width */
			shift = 8 + shift;	/* enforce right rotate */
		}
		else if (shift > 0)
			width = aligns + w + 8; /* enfore drain at right edge */

		lmasks = SFBCOPYALL1 << aligns;
		rmasks = SFBCOPYALL1 >> (-width & SFBCOPYBITMASK);
		lmaskd = SFBCOPYALL1 << alignd;
		rmaskd = SFBCOPYALL1 >> (-(w + alignd) & SFBCOPYBITMASK);

		if (w + alignd <= SFBCOPYBITS)
			goto singlewrite;

		SFBPIXELSHIFT(sfb, shift);
		w = width;
		sq = sp;
		dq = dp;
		while (height > 0) {
			*(u_int32_t *)sp = lmasks;
WRITE_MB();
			*(u_int32_t *)dp = lmaskd;
WRITE_MB();
			width -= 2 * SFBCOPYBITS;
			while (width > 0) {
				sp += SFBCOPYBYTESDONE;
				dp += SFBCOPYBYTESDONE;
				*(u_int32_t *)sp = SFBCOPYALL1;
WRITE_MB();
				*(u_int32_t *)dp = SFBCOPYALL1;
WRITE_MB();
				width -= SFBCOPYBITS;
			}
			sp += SFBCOPYBYTESDONE;
			dp += SFBCOPYBYTESDONE;
			*(u_int32_t *)sp = rmasks;
WRITE_MB();
			*(u_int32_t *)dp = rmaskd;
WRITE_MB();

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	/* copy backward (right-to-left) */
	else {
		caddr_t sq, dq;

		shift = alignd - aligns;
		if (shift > 0) {
			shift = shift - 8;
			w += 8;
		}
		width = w + aligns;
		lmasks = SFBCOPYALL1 << aligns;
		rmasks = SFBCOPYALL1 >> (-width & SFBCOPYBITMASK);
		lmaskd = SFBCOPYALL1 << alignd;
		rmaskd = SFBCOPYALL1 >> (-(w + alignd) & SFBCOPYBITMASK);

		if (w + alignd <= SFBCOPYBITS)
			goto singlewrite;

		SFBPIXELSHIFT(sfb, shift);
		w = width;
		sq = (sp += width);
		dq = (dp += width);
		while (height > 0) {
			*(u_int32_t *)sp = rmasks;
WRITE_MB();
			*(u_int32_t *)dp = rmaskd;
WRITE_MB();
			width -= 2 * SFBCOPYBITS;
			while (width > 0) {
				sp -= SFBCOPYBYTESDONE;
				dp -= SFBCOPYBYTESDONE;
				*(u_int32_t *)sp = SFBCOPYALL1;
WRITE_MB();
				*(u_int32_t *)dp = SFBCOPYALL1;
WRITE_MB();
				width -= SFBCOPYBITS;
			}
			sp -= SFBCOPYBYTESDONE;
			dp -= SFBCOPYBYTESDONE;
			*(u_int32_t *)sp = lmasks;
WRITE_MB();
			*(u_int32_t *)dp = lmaskd;
WRITE_MB();

			sp = (sq += scanspan);
			dp = (dq += scanspan);
			width = w;
			height--;
		}
	}
	SFBMODE(sfb, MODE_SIMPLE);
	SFBPIXELSHIFT(sfb, 0);
	return;

singlewrite:
	SFBPIXELSHIFT(sfb, shift);
	lmasks = lmasks & rmasks;
	lmaskd = lmaskd & rmaskd;
	while (height > 0) {
		*(u_int32_t *)sp = lmasks;
WRITE_MB();
		*(u_int32_t *)dp = lmaskd;
WRITE_MB();
		sp += scanspan;
		dp += scanspan;
		height--;
	}
	SFBMODE(sfb, MODE_SIMPLE);
	SFBPIXELSHIFT(sfb, 0);
}

/*
 * Clear characters in a line.
 */
static void
TEXTOP(erasecols)(id, row, startcol, ncols, attr)
	void *id;
	int row, startcol, ncols;
	long attr;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, startx, height, count, y;

	scanspan = rap->linelongs * 4;
	y = rc->rc_yorigin + rc->rc_font->height * row;
	startx = rc->rc_xorigin + rc->rc_font->width * startcol;
	height = rc->rc_font->height;
	count = rc->rc_font->width * ncols - 1;

	p = (caddr_t)rap->pixels + y * scanspan + startx * SFBPIXELBYTES;
	sfb = rap->data;

	SFBMODE(sfb, MODE_TRANSPARENTFILL);
	SFBPLANEMASK(sfb, ~0);
	SFBFG(sfb, 0);			/* fill with bg color */
	SFBDATA(sfb, ~0);		/* fill mask */
	while (height > 0) {
		SFBADDRESS(sfb, (long)p);
		SFBBCONT(sfb, count);
		p += scanspan;
		height--;
	}
	SFBMODE(sfb, MODE_SIMPLE);
}

/*
 * Copy lines.
 */
static void
TEXTOP(copyrows)(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, offset, srcy, height, width, align, w;
	u_int32_t lmask, rmask;

	scanspan = rap->linelongs * 4;
	height = rc->rc_font->height * nrows;
	offset = (dstrow - srcrow) * scanspan * rc->rc_font->height;
	srcy = rc->rc_yorigin + rc->rc_font->height * srcrow;
	if (srcrow < dstrow && srcrow + nrows > dstrow) {
		scanspan = -scanspan;
		srcy += height;
	}

	p = (caddr_t)(rap->pixels + srcy * rap->linelongs);
	p += rc->rc_xorigin * SFBPIXELBYTES;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	align /= SFBPIXELBYTES;
	w = rc->rc_font->width * rc->rc_maxcol;
	width = w + align;
	lmask = SFBCOPYALL1 << align;
	rmask = SFBCOPYALL1 >> (-width & SFBCOPYBITMASK);
	sfb = rap->data;

	SFBMODE(sfb, MODE_COPY);
	SFBPLANEMASK(sfb, ~0);
	SFBPIXELSHIFT(sfb, 0);
	if (width <= SFBCOPYBITS) {
		/* never happens */;
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			*(u_int32_t *)p = lmask;
			*(u_int32_t *)(p + offset) = lmask;
			width -= 2 * SFBCOPYBITS;
			while (width > 0) {
				p += SFBCOPYBYTESDONE;
				*(u_int32_t *)p = SFBCOPYALL1;
				*(u_int32_t *)(p + offset) = SFBCOPYALL1;
				width -= SFBCOPYBITS;
			}
			p += SFBCOPYBYTESDONE;
			*(u_int32_t *)p = rmask;
			*(u_int32_t *)(p + offset) = rmask;

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
	SFBMODE(sfb, MODE_SIMPLE);
}

#if 0

#define	SFBCOPY64BYTESDONE	8
#define	SFBCOPY64BITS		64
#define	SFBCOPY64SRC(p, v) \
		(*((u_int32_t *)(p) + TGA_REG_GCSR) = (long)(v))
#define	SFBCOPY64DST(p, v) \
		(*((u_int32_t *)(p) + TGA_REG_GCDR) = (long)(v))

void
TEXTOP(copyrows)(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p, q;
	int scanspan, offset, srcy, height, width, w;
	u_int32_t rmask;

	scanspan = rap->linelongs * 4;
	height = rc->rc_font->height * nrows;
	width = rc->rc_font->width * rc->rc_maxcol;
	offset = (dstrow - srcrow) * scanspan * rc->rc_font->height;
	srcy = rc->rc_yorigin + rc->rc_font->height * srcrow;
	if (srcrow < dstrow && srcrow + nrows > dstrow) {
		scanspan = -scanspan;
		srcy += height;
	}

	p = (caddr_t)(rap->pixels + srcy * rap->linelongs);
	p += rc->rc_xorigin * SFBPIXELBYTES;
	rmask = SFBCOPYALL1 >> (-width & SFBCOPYBITMASK);
	sfb = rap->data;
	w = width;
	q = p;

	SFBMODE(sfb, MODE_COPY);
	SFBPLANEMASK(sfb, ~0);
	SFBPIXELSHIFT(sfb, 0);
	
	if (width <= SFBCOPYBITS)
		; /* never happens */
	else if (width < SFBCOPY64BITS) {
		; /* unlikely happens */

	}
	else {
		while (height > 0) {
			while (width >= SFBCOPY64BITS) {
				SFBCOPY64SRC(sfb, p);
				SFBCOPY64DST(sfb, p + offset);
				p += SFBCOPY64BYTESDONE;
				width -= SFBCOPY64BITS;
			}
			if (width >= SFBCOPYBITS) {
				*(u_int32_t *)p = SFBCOPYALL1;
				*(u_int32_t *)(p + offset) = SFBCOPYALL1;
				p += SFBCOPYBYTESDONE;
				width -= SFBCOPYBITS;
			}
			if (width > 0) {
				*(u_int32_t *)p = rmask;
				*(u_int32_t *)(p + offset) = rmask;
			}

			p = (q += scanspan);
			width = w;
			height--;
		}
	}
	SFBMODE(sfb, MODE_SIMPLE);
}
#endif

/*
 * Erase lines.
 */
void
TEXTOP(eraserows)(id, startrow, nrows, attr)
	void *id;
	int startrow, nrows;
	long attr;
{
	struct rcons *rc = id;
	struct raster *rap = rc->rc_sp;
	caddr_t sfb, p;
	int scanspan, starty, height, width, align, w;
	u_int32_t lmask, rmask;

	scanspan = rap->linelongs * 4;
	starty = rc->rc_yorigin + rc->rc_font->height * startrow;
	height = rc->rc_font->height * nrows;

	p = (caddr_t)rap->pixels + starty * scanspan;
	p += rc->rc_xorigin * SFBPIXELBYTES;
	align = (long)p & SFBALIGNMASK;
	p -= align;
	align /= SFBPIXELBYTES;
	w = rc->rc_font->width * rc->rc_maxcol;
	width = w + align;
	lmask = SFBSTIPPLEALL1 << align;
	rmask = SFBSTIPPLEALL1 >> (-width & SFBSTIPPLEBITMASK);
	sfb = rap->data;

	SFBMODE(sfb, MODE_TRANSPARENTSTIPPLE);
	SFBPLANEMASK(sfb, ~0);
	SFBFG(sfb, 0);				/* fill with bg color */
	if (width <= SFBSTIPPLEBITS) {
		/* never happens */;
	}
	else {
		caddr_t q = p;
		while (height > 0) {
			*(u_int32_t *)p = lmask;
WRITE_MB();
			width -= 2 * SFBSTIPPLEBITS;
			while (width > 0) {
				p += SFBSTIPPLEBYTESDONE;
				*(u_int32_t *)p = SFBSTIPPLEALL1;
WRITE_MB();
				width -= SFBSTIPPLEBITS;
			}
			p += SFBSTIPPLEBYTESDONE;
			*(u_int32_t *)p = rmask;
WRITE_MB();

			p = (q += scanspan);
			width = w + align;
			height--;
		}
	}
	SFBMODE(sfb, MODE_SIMPLE);
}
