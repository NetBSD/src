/* $NetBSD: rasops1.c,v 1.2 1999/04/13 00:40:09 ad Exp $ */

/*
 * Copyright (c) 1999 Andy Doran <ad@NetBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_rasops.h"
#ifdef RASOPS1

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops1.c,v 1.2 1999/04/13 00:40:09 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <machine/endian.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

static void	rasops1_putchar __P((void *, int, int col, u_int, long));
static void	rasops1_putchar8 __P((void *, int, int col, u_int, long));
static void	rasops1_putchar16 __P((void *, int, int col, u_int, long));
static void	rasops1_copycols __P((void *, int, int, int, int));
static void	rasops1_erasecols __P((void *, int, int, int));
static void	rasops1_erasecols8 __P((void *, int, int, int));
static void	rasops1_eraserows __P((void *, int, int));
static int32_t	rasops1_fg_color __P((long));
static int32_t	rasops1_bg_color __P((long));
static void	rasops1_do_cursor __P((struct rasops_info *));
static void	rasops1_do_cursor8 __P((struct rasops_info *));

void	rasops1_init __P((struct rasops_info *ri));

#if BYTE_ORDER == LITTLE_ENDIAN
static u_int32_t rightmask[32] = {
#else
static u_int32_t leftmask[32] = {
#endif
    0x00000000, 0x80000000, 0xc0000000, 0xe0000000,
    0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000,
    0xff000000, 0xff800000, 0xffc00000, 0xffe00000,
    0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000,
    0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000,
    0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
    0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0,
    0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe,
};

#if BYTE_ORDER == LITTLE_ENDIAN
static u_int32_t leftmask[32] = {
#else
static u_int32_t rightmask[32] = {
#endif
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
};


/*
 * Initalize rasops_info struct for this colordepth.
 */
void
rasops1_init(ri)
	struct rasops_info *ri;
{

	switch (ri->ri_font->fontwidth) {
	case 8:
		ri->ri_ops.putchar = rasops1_putchar8;
		break;
	case 16:
		ri->ri_ops.putchar = rasops1_putchar16;
		break;
	default:
		/* 
		 * XXX we don't support anything other than 8 and 16 pel
		 * wide fonts yet. rasops1_copycols needs to be finished, the
		 * other routines will need to be checked.
		 */
		panic("rasops1_init: not completed - see this location in src for details");
		ri->ri_ops.putchar = rasops1_putchar;
		break;
	}
		
	if (ri->ri_font->fontwidth & 7) {
		ri->ri_ops.erasecols = rasops1_erasecols;
		ri->ri_ops.copycols = rasops1_copycols;
		ri->ri_do_cursor = rasops1_do_cursor;
	} else {
		ri->ri_ops.erasecols = rasops1_erasecols8;
		/* use default copycols */
		ri->ri_do_cursor = rasops1_do_cursor8;
	}

	ri->ri_ops.eraserows = rasops1_eraserows;		
}


/*
 * Get foreground color from attribute and copy across all 4 bytes
 * in a int32_t.
 */
static __inline__ int32_t
rasops1_fg_color(long attr)
{
	
	return (attr & 0x0f000000) ? 0xffffffff : 0;
}


/*
 * Get background color from attribute and copy across all 4 bytes
 * in a int32_t.
 */
static __inline__ int32_t
rasops1_bg_color(long attr)
{

	return (attr & 0x000f0000) ? 0xffffffff : 0;
}


/*
 * Paint a single character. This is the generic version.
 */
static void
rasops1_putchar(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{

	/* XXX i need implemention */
}


/*
 * Paint a single character. This is for 8-pixel wide fonts.
 */
static void
rasops1_putchar8(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	int height, fs, rs, bg, fg;
	struct rasops_info *ri;
	u_char *fr, *rp;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if (row < 0 || row >= ri->ri_rows)
		return;

	if (col < 0 || col >= ri->ri_cols)
		return;
#endif

	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	height = ri->ri_font->fontheight;
	rs = ri->ri_stride;
	
	bg = (attr & 0x000f0000) ? 0xff : 0;
	fg = (attr & 0x0f000000) ? 0xff : 0;
	
	/* If fg and bg match this becomes a space character */
	if (fg == bg || uc == ' ') {
		while (height--) {
			*rp = bg;
			rp += rs;
		}
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;
		
		/* NOT fontbits if bg is white */
		if (bg) {
			while (height--) {
				*rp = ~*fr;
				fr += fs;
				rp += rs;
			}
		} else {
			while (height--) {
				*rp = *fr;
				fr += fs;
				rp += rs;
			}
		}
	}

	/* Do underline */
	if (attr & 1)
		rp[-(ri->ri_stride << 1)] = fg;
}


/*
 * Paint a single character. This is for 16-pixel wide fonts.
 */
static void
rasops1_putchar16(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	int height, fs, rs, bg, fg;
	struct rasops_info *ri;
	u_char *fr, *rp;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if (row < 0 || row >= ri->ri_rows)
		return;

	if (col < 0 || col >= ri->ri_cols)
		return;
#endif

	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	height = ri->ri_font->fontheight;
	rs = ri->ri_stride;
	
	bg = (attr & 0x000f0000) ? 0xffff : 0;
	fg = (attr & 0x0f000000) ? 0xffff : 0;
	
	/* If fg and bg match this becomes a space character */
	if (fg == bg || uc == ' ') {
		while (height--) {
			*(int16_t *)rp = bg;
			rp += rs;
		}
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;
		
		/* NOT fontbits if bg is white */
		if (bg) {
			while (height--) {
				rp[0] = ~fr[0];
				rp[1] = ~fr[1];
				fr += fs;
				rp += rs;
			}
		} else {
			while (height--) {
				rp[0] = fr[0];
				rp[1] = fr[1];
				fr += fs;
				rp += rs;
			}
		}
	}

	/* Do underline */
	if (attr & 1)
		*(int16_t *)(rp - (ri->ri_stride << 1)) = fg;
}


/*
 * Erase columns.
 */
static void
rasops1_erasecols(cookie, row, col, num, attr)
	void *cookie;
	int row, num;
	long attr;
{
	int32_t *dp, *rp, lmask, rmask, lclr, rclr, clr;
	struct rasops_info *ri;
	int nint, height, cnt;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if (row < 0 || row > ri->ri_rows)
		return;

	if (col < 0) {
		num += col;
		col = 0;
	}

	if ((col + num) > ri->ri_cols)
		num = ri->ri_cols - col;
	
	if (num <= 0)
		return;
#endif
	col *= ri->ri_fontwidth;
	num *= ri->ri_fontwidth;

	/*
	 * lmask: mask for leftmost int32
	 * rmask: mask for rightmost int32
	 * nint: number of full int32s
	 */ 
	lmask = leftmask[col & 31];
	rmask = rightmask[(col + num) & 31];
	nint = ((col + num) & ~31) - ((col + 31) & ~31) >> 5;

	/* Merge both masks if the span is encapsulated within one int32 */
	if (col & ~31 == (col + num) & ~31) {
		lmask &= rmask;
		rmask = 0;
	}
	
	clr = rasops1_bg_color(attr);	
	lclr = clr & lmask;
	rclr = clr & rmask;
	lmask = ~lmask;
	rmask = ~rmask;
	height = ri->ri_fontheight;
	rp = ri->ri_bits + row*ri->ri_yscale + ((col >> 3) & ~3);
	
	while (height--) {
		dp = rp;
		DELTA(rp, ri->ri_stride, int32_t *);
	
		if (lmask != 0xffffffffU)
			*dp++ = (*dp & lmask) | lclr;
			
		for (cnt = nint; cnt; cnt--)
			*dp++ = clr;

		if (rmask != 0xffffffffU)
			*dp = (*dp & rmask) | rclr;
	}
}


/*
 * Erase columns for fonts that are a multiple of 8 pels in width.
 */
static void
rasops1_erasecols8(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	struct rasops_info *ri;
	int32_t *dst;
	u_char *dstb, *rp;
	int clr, height, cnt, slop1, slop2;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if (row < 0 || row >= ri->ri_rows)
		return;

	if (col < 0) {
		num += col;
		col = 0;
	}

	if ((col + num) > ri->ri_cols)
		num = ri->ri_cols - col;
	
	if (num <= 0)
		return;
#endif
		
	num = num * ri->ri_xscale;
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	clr = rasops1_bg_color(attr);
	height = ri->ri_font->fontheight;
	
	slop1 = (int)rp & 3;
	slop2 = (num - slop1) & 3;
	num = (num - slop1 - slop2) >> 2;

	while (height--) {
		dstb = rp;
		rp += ri->ri_stride;
	
		/* Align span to 4 bytes */
		for (cnt = slop1; cnt; cnt--)
			*dstb++ = (u_char)clr;
	
		dst = (int32_t *)dstb;
		
		/* Write 4 bytes per loop */	
		for (cnt = num; cnt; cnt--)
			*dst++ = clr;
	
		/* Write unaligned trailing slop */ 
		if (slop2 == 0)
			continue;
			
		dstb = (u_char *)dst;
		
		for (cnt = slop2; cnt; cnt--)
			*dstb++ = (u_char)clr;
	}
}


/*
 * Actually paint the cursor.
 */
static void
rasops1_do_cursor(ri)
	struct rasops_info *ri;
{
	int32_t *dp, *rp, lmask, rmask;
	int height, row, col;
	
	row = ri->ri_crow;
	col = ri->ri_ccol * ri->ri_fontwidth;

	/*
	 * lmask: mask for leftmost int32
	 * rmask: mask for rightmost int32
	 */ 
	lmask = leftmask[col & 31];
	rmask = rightmask[(col + ri->ri_fontwidth) & 31];

	/* Merge both masks if the span is encapsulated within one int32 */
	if (col & ~31 == (col + ri->ri_fontwidth) & ~31) {
		lmask &= rmask;
		rmask = 0;
	}
	
	lmask = ~lmask;
	rmask = ~rmask;
	height = ri->ri_fontheight;
	rp = ri->ri_bits + row*ri->ri_yscale + ((col >> 3) & ~3);
	
	while (height--) {
		dp = rp;
		DELTA(rp, ri->ri_stride, int32_t *);
	
		*dp++ = ((*dp & lmask) ^ lmask) | (*dp & ~lmask);
			
		if (rmask != 0xffffffffU)
			*dp = ((*dp & rmask) ^ rmask) | (*dp & ~rmask);
	}
}


/*
 * Paint cursors that are a multiple of 8 pels in size.
 */
static void
rasops1_do_cursor8(ri)
	struct rasops_info *ri;
{
	int width, height, cnt;
	u_char *dp, *rp;
	
	height = ri->ri_fontheight;
	width = ri->ri_fontwidth >> 3;
	rp = ri->ri_bits + ri->ri_crow * ri->ri_yscale + 
	    ri->ri_ccol * ri->ri_xscale;
	
	while (height--) {
		dp = rp;
		rp += ri->ri_stride;
		
		for (cnt = width; cnt; cnt--)
			*dp++ ^= 0xff;
	}
}


/*
 * Erase rows. 
 */
static void
rasops1_eraserows(cookie, row, num, attr)
	void *cookie;
	int row, num;
	long attr;
{
	struct rasops_info *ri;
	int32_t *dp, clr;
	int n8, n1, cnt;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING
	if (row < 0) {
		num += row;
		row = 0;
	}

	if ((row + num) > ri->ri_rows)
		num = ri->ri_rows - row;
	
	if (num <= 0)
		return;
#endif
		
	num *= ri->ri_font->fontheight;
	dp = (int32_t *)(ri->ri_bits + row * ri->ri_yscale);
	clr = rasops1_bg_color(attr);

	n8 = ri->ri_emustride >> 5;
	n1 = (ri->ri_emustride >> 2) & 7;
	
	while (num--) {
		for (cnt = n8; cnt; cnt--) {
			dp[0] = clr;
			dp[1] = clr;
			dp[2] = clr;
			dp[3] = clr;
			dp[4] = clr;
			dp[5] = clr;
			dp[6] = clr;
			dp[7] = clr;
			dp += 8;
		}
		
		for (cnt = n1; cnt; cnt--)
			*dp++ = clr;
			
		DELTA(dp, ri->ri_delta, int32_t *);
	}
}


/*
 * Copy columns. This is slow. Ick! You'd better use a font with
 * a width that's a multiple of 8 pels, otherwise this is used...
 */
static void
rasops1_copycols(cookie, row, src, src)
	void *cookie;
	int row, src, dst;
{
#ifdef notyet
	struct rasops_info *ri;
	int32_t *dp, *drp, *sp, *srp, lmask, rmask;
	int nint, height, cnt;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if (row < 0 || row >= ri->ri_rows)
		return;

	if (col < 0) {
		num += col;
		col = 0;
	}

	if ((col + num) > ri->ri_cols)
		num = ri->ri_cols - col;
	
	if (num <= 0)
		return;
#endif
	src *= ri->ri_fontwidth;
	dst *= ri->ri_fontwidth;
	num *= ri->ri_fontwidth;

	/*
	 * lmask: mask for leftmost int32
	 * rmask: mask for rightmost int32
	 * nint: number of full int32s
	 */ 
	lmask = leftmask[col & 31];
	rmask = rightmask[(col + num) & 31];
	nint = ((col + num) & ~31) - ((col + 31) & ~31) >> 5;

	/* Merge both masks if the span is encapsulated within one int32 */
	if (col & ~31 == (col + num) & ~31) {
		lmask &= rmask;
		rmask = 0;
	}
	
	lmask = ~lmask;
	rmask = ~rmask;
	height = ri->ri_fontheight;
	drp = ri->ri_bits + row*ri->ri_yscale + ((dst >> 3) & ~3);
	srp = ri->ri_bits + row*ri->ri_yscale + ((src >> 3) & ~3);
	
	while (height--) {
		dp = rp;
		DELTA(rp, ri->ri_stride, int32_t *);
	
		if (lmask != 0xffffffffU)
			*dp++ = (*dp & lmask) | lclr;
			
		for (cnt = nint; cnt; cnt--)
			*dp++ = clr;

		if (rmask != 0xffffffffU)
			*dp = (*dp & rmask) | rclr;
	}
#endif
}

#endif /* RASOPS1 */
