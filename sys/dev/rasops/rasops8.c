/* $NetBSD: rasops8.c,v 1.1 1999/04/13 00:18:01 ad Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran <ad@NetBSD.org>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include "opt_rasops.h"
#ifdef RASOPS8

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops8.c,v 1.1 1999/04/13 00:18:01 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops8_putchar __P((void *, int, int, u_int, long attr));
static void 	rasops8_putchar8 __P((void *, int, int, u_int, long attr));
static void 	rasops8_putchar12 __P((void *, int, int, u_int, long attr));
static void 	rasops8_putchar16 __P((void *, int, int, u_int, long attr));
static void 	rasops8_erasecols __P((void *, int, int, int, long));
static void 	rasops8_eraserows __P((void *, int, int, long));
static void	rasops8_makestamp __P((long));
static int32_t	rasops8_bg_color __P((long));
static void	rasops8_do_cursor __P((struct rasops_info *));
void		rasops8_init __P((struct rasops_info *ri));

/* 
 * 4x1 stamp for optimized character blitting 
 */
static int32_t	stamp[16];
static long	stamp_attr;
static int	stamp_mutex;	/* XXX see note in README */

/*
 * XXX this confuses the hell out of gcc2 (not egcs) which always insists
 * that the shift count is negative.
 *
 * offset = STAMP_SHIFT(fontbits, nibble #) & STAMP_MASK
 * destination = STAMP_READ(offset)
 */
#define STAMP_SHIFT(fb,n)	((n*4-2) >= 0 ? (fb)>>(n*4-2):(fb)<<-(n*4-2))
#define STAMP_MASK		(15 << 2)
#define STAMP_READ(o)		(*(int32_t *)((caddr_t)stamp + (o)))


/*
 * Initalize a 'rasops_info' descriptor for this depth.
 */
void
rasops8_init(ri)
	struct rasops_info *ri;
{
	
	switch (ri->ri_font->fontwidth) {
	case 8:
		ri->ri_ops.putchar = rasops8_putchar8;
		break;
		
	case 12:
		ri->ri_ops.putchar = rasops8_putchar12;
		break;
		
	case 16:
		ri->ri_ops.putchar = rasops8_putchar16;
		break;
		
	default:
		ri->ri_ops.putchar = rasops8_putchar;
		break;
	}
	
	ri->ri_ops.erasecols = rasops8_erasecols;
	ri->ri_ops.eraserows = rasops8_eraserows;		
	ri->ri_do_cursor = rasops8_do_cursor;
}


/*
 * Get background color from attribute and copy across all 4 bytes
 * of an int32_t.
 */
static __inline__ int32_t
rasops8_bg_color(attr)
	long attr;
{
	int32_t bg;
	
	bg = ((int32_t)attr >> 16) & 15;
	return bg | (bg << 8) | (bg << 16) | (bg << 24);
}


/*
 * Actually turn the cursor on or off. This does the dirty work for
 * rasops_cursor().
 */
static void
rasops8_do_cursor(ri)
	struct rasops_info *ri;
{
	u_char *dp, *rp;
	int full1, height, cnt, slop1, slop2, row, col;
	
	row = ri->ri_crow;
	col = ri->ri_ccol;
	
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	height = ri->ri_font->fontheight;
	
	slop1 = (int)rp & 3;
	slop2 = (ri->ri_xscale - slop1) & 3;
	full1 = (ri->ri_xscale - slop1 - slop2) >> 2;

	while (height--) {
		dp = rp;
		rp += ri->ri_stride;
	
		for (cnt = slop1; cnt; cnt--)
			*dp++ ^= 0xff;
	
		for (cnt = full1; cnt; cnt--) {
			*(int32_t *)dp ^= 0xffffffff;
			dp += 4;
		}
	
		for (cnt = slop2; cnt; cnt--)
			*dp++ ^= 0xff;
	}
}


/*
 * Paint a single character.
 */
static void
rasops8_putchar(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	u_char *dp, *rp, *fr, clr[2];
	int width, height, cnt, fs, fb;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	/* Catches 'row < 0' case too */ 
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;

	height = ri->ri_font->fontheight;
	width = ri->ri_font->fontwidth;
	clr[0] = (u_char)(attr >> 16);
	clr[1] = (u_char)(attr >> 24);
		
	if (uc == ' ') {
		while (height--) {
			dp = rp;
			rp += ri->ri_stride;
			
			for (cnt = width; cnt; cnt--)
				*dp++ = clr[0];
		}
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;

		while (height--) {
			dp = rp;
			fb = fr[3] | (fr[2] << 8) | (fr[1] << 16) | (fr[0] << 24);
			fr += fs;
			rp += ri->ri_stride;
			
			for (cnt = width; cnt; cnt--) {
				*dp++ = clr[(fb >> 31) & 1];
				fb <<= 1;
			}
		}
	}	

	/* Do underline */
	if (attr & 1) {
		rp -= (ri->ri_stride << 1);

		while (width--)
			*rp++ = clr[1];
	}	

}


/*
 * Recompute the 4x1 blitting stamp.
 */
static void
rasops8_makestamp(long attr)
{
	int i;
	int32_t fg, bg;
	
	fg = (attr >> 24) & 15;
	bg = (attr >> 16) & 15;
	stamp_attr = attr;
	
	for (i = 0; i < 16; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
		stamp[i] = (i & 8 ? fg : bg);
		stamp[i] |= ((i & 4 ? fg : bg) << 8);
		stamp[i] |= ((i & 2 ? fg : bg) << 16);
		stamp[i] |= ((i & 1 ? fg : bg) << 24);
#else
		stamp[i] = (i & 1 ? fg : bg);
		stamp[i] |= ((i & 2 ? fg : bg) << 8);
		stamp[i] |= ((i & 4 ? fg : bg) << 16);
		stamp[i] |= ((i & 8 ? fg : bg) << 24);
#endif
	}
}


/*
 * Paint a single character. This is for 8-pixel wide fonts.
 */
static void
rasops8_putchar8(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	int height, fs;
	int32_t *rp;
	u_char *fr;
	
	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops8_putchar(cookie, row, col, uc, attr);
		return;
	}

	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows) {
		stamp_mutex--;
		return;
	}

	if ((unsigned)col >= (unsigned)ri->ri_cols) {
		stamp_mutex--;
		return;
	}
#endif

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops8_makestamp(attr);
	
	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	height = ri->ri_font->fontheight;
	
	if (uc == ' ') {
		while (height--) {
			rp[0] = stamp[0];
			rp[1] = stamp[0];
			DELTA(rp, ri->ri_stride, int32_t *);
		}	
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;
		
		while (height--) {
			rp[0] = STAMP_READ(STAMP_SHIFT(fr[0], 1) & STAMP_MASK);
			rp[1] = STAMP_READ(STAMP_SHIFT(fr[0], 0) & STAMP_MASK);

			fr += fs;
			DELTA(rp, ri->ri_stride, int32_t *);	
		}
	}

	/* Do underline */
	if (attr & 1) {
		DELTA(rp, -(ri->ri_stride << 1), int32_t *);	
		rp[0] = stamp[15];
		rp[1] = stamp[15];
	}	
	
	stamp_mutex--;	
}


/*
 * Paint a single character. This is for 12-pixel wide fonts.
 */
static void
rasops8_putchar12(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	int height, fs;
	int32_t *rp;
	u_char *fr;
	
	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops8_putchar(cookie, row, col, uc, attr);
		return;
	}

	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows) {
		stamp_mutex--;
		return;
	}

	if ((unsigned)col >= (unsigned)ri->ri_cols) {
		stamp_mutex--;
		return;
	}
#endif

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops8_makestamp(attr);
	
	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	height = ri->ri_font->fontheight;
	
	if (uc == ' ') {
		while (height--) {
			rp[0] = stamp[0];
			rp[1] = stamp[0];
			rp[2] = stamp[0];
			DELTA(rp, ri->ri_stride, int32_t *);	
		}	
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;
	
		while (height--) {
			rp[0] = STAMP_READ(STAMP_SHIFT(fr[0], 1) & STAMP_MASK);
			rp[1] = STAMP_READ(STAMP_SHIFT(fr[0], 0) & STAMP_MASK);
			rp[2] = STAMP_READ(STAMP_SHIFT(fr[1], 1) & STAMP_MASK);
			
			fr += fs;
			DELTA(rp, ri->ri_stride, int32_t *);	
		}
	}	

	/* Do underline */
	if (attr & 1) {
		DELTA(rp, -(ri->ri_stride << 1), int32_t *);	
		rp[0] = stamp[15];
		rp[1] = stamp[15];
		rp[2] = stamp[15];
	}	
	
	stamp_mutex--;
}


/*
 * Paint a single character. This is for 16-pixel wide fonts.
 */
static void
rasops8_putchar16(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	int height, fs;
	int32_t *rp;
	u_char *fr;

	/* Can't risk remaking the stamp if it's already in use */
	if (stamp_mutex++) {
		stamp_mutex--;
		rasops8_putchar(cookie, row, col, uc, attr);
		return;
	}

	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows) {
		stamp_mutex--;
		return;
	}

	if ((unsigned)col >= (unsigned)ri->ri_cols) {
		stamp_mutex--;
		return;
	}
#endif

	/* Recompute stamp? */
	if (attr != stamp_attr)
		rasops8_makestamp(attr);
	
	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	height = ri->ri_font->fontheight;
	
	if (uc == ' ') {
		while (height--) {
			rp[0] = stamp[0];
			rp[1] = stamp[0];
			rp[2] = stamp[0];
			rp[3] = stamp[0];
			DELTA(rp, ri->ri_stride, int32_t *);	
		}	
	} else {
		uc -= ri->ri_font->firstchar;
		fr = (u_char *)ri->ri_font->data + uc * ri->ri_fontscale;
		fs = ri->ri_font->stride;
	
		while (height--) {
			rp[0] = STAMP_READ(STAMP_SHIFT(fr[0], 1) & STAMP_MASK);
			rp[1] = STAMP_READ(STAMP_SHIFT(fr[0], 0) & STAMP_MASK);
			rp[2] = STAMP_READ(STAMP_SHIFT(fr[1], 1) & STAMP_MASK);
			rp[3] = STAMP_READ(STAMP_SHIFT(fr[1], 0) & STAMP_MASK);

			fr += fs;
			DELTA(rp, ri->ri_stride, int32_t *);	
		}
	}	

	/* Do underline */
	if (attr & 1) {
		DELTA(rp, -(ri->ri_stride << 1), int32_t *);	
		rp[0] = stamp[15];
		rp[1] = stamp[15];
		rp[2] = stamp[15];
		rp[3] = stamp[15];
	}	
	
	stamp_mutex--;
}


/*
 * Erase rows. 
 */
static void
rasops8_eraserows(cookie, row, num, attr)
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
	clr = rasops8_bg_color(attr);

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
 * Erase columns.
 */
static void
rasops8_erasecols(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	int n8, clr, height, cnt, slop1, slop2;
	struct rasops_info *ri;
	int32_t *dst;
	u_char *dstb, *rp;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows)
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
	clr = rasops8_bg_color(attr);
	height = ri->ri_font->fontheight;
	
	slop1 = (int)rp & 3;
	slop2 = (num - slop1) & 3;
	num -= slop1 + slop2;
	n8 = num >> 5;
	num = (num >> 2) & 7;

	while (height--) {
		dstb = rp;
		rp += ri->ri_stride;
	
		/* Align span to 4 bytes */
		for (cnt = slop1; cnt; cnt--)
			*dstb++ = (u_char)clr;
	
		dst = (int32_t *)dstb;
		
		/* Write 32 bytes per loop */
		for (cnt = n8; cnt; cnt--) {
			dst[0] = clr;
			dst[1] = clr;
			dst[2] = clr;
			dst[3] = clr;
			dst[4] = clr;
			dst[5] = clr;
			dst[6] = clr;
			dst[7] = clr;
			dst += 8;
		}
		
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

#endif /* RASOPS8 */
