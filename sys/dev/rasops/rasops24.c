/* $NetBSD: rasops24.c,v 1.1 1999/04/13 00:18:00 ad Exp $ */

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops24.c,v 1.1 1999/04/13 00:18:00 ad Exp $");

#include "opt_rasops.h"
#ifdef RASOPS24
#error This is dangerously incomplete.

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops24_putchar __P((void *, int, int, u_int, long attr));
static void 	rasops24_erasecols __P((void *, int, int, int, long));
static void 	rasops24_eraserows __P((void *, int, int, long));
static int	rasops24_alloc_attr __P((void *, int, int, int, long *));
static void	rasops24_make_bgstamp __P((int, u_char *));
static int32_t	rasops24_fg_color __P((struct rasops_info *, long));
static int32_t	rasops24_bg_color __P((struct rasops_info *, long));
static void	rasops24_do_cursor __P((struct rasops_info *));

void	rasops24_init __P((struct rasops_info *ri));

/*
 * Initalize rasops_info struct for this colordepth.
 */
void
rasops24_init(ri)
	struct rasops_info *ri;
{

	switch (ri->ri_font->fontwidth) {
	case 8:
		ri->ri_ops.putchar = rasops15_putchar8;
		break;
		
	case 12:
		ri->ri_ops.putchar = rasops15_putchar12;
		break;
		
	case 16:
		ri->ri_ops.putchar = rasops15_putchar16;
		break;
		
	default:
		ri->ri_ops.putchar = rasops15_putchar;
		break;
	}
	
	/* Select defaults for color positions if none selected */
	if (ri->ri_rnum == 0) {
		ri->ri_rnum = 8;
		ri->ri_gnum = 8;
		ri->ri_bnum = 8;
		ri->ri_rpos = 0;
		ri->ri_gpos = 8;
		ri->ri_bpos = 16;
	}
		
	ri->ri_ops.erasecols = rasops25_erasecols;
	ri->ri_ops.eraserows = rasops25_eraserows;		
	ri->ri_do_cursor = rasops25_do_cursor;
	rasops_init_devcmap(ri);
}

/*
 * Get foreground color from attribute and copy across all 4 bytes
 * in a int32_t.
 */
static __inline__ int32_t
rasops24_fg_color(ri, attr)
	struct rasops_info *ri;
	long attr;
{
	int32_t fg;
	
	fg = ri->ri_devcmap[((u_int)attr >> 24) & 15];
	
	/* Copy across all 4 bytes if the color is gray */
	if (attr & 2)
		fg |= fg << 8;
	
	return fg;
}


/*
 * Get background color from attribute and copy across all 4 bytes
 * in a int32_t.
 */
static __inline__ int32_t
rasops24_bg_color(ri, attr)
	struct rasops_info *ri;
	long attr;
{
	int32_t bg;
	
	bg = ri->ri_devcmap[((u_int)attr >> 16) & 15];
	
	/* Copy across all 4 bytes if the color is gray */
	if (attr & 4)
		bg |= bg << 8;
	
	return bg;
}


/*
 * Actually turn the cursor on or off. This does the dirty work for
 * rasops_cursor().
 */
static void
rasops24_do_cursor(ri)
	struct rasops_info *ri;
{
	int full1, height, cnt, slop1, slop2, row, col;
	u_char *dp, *rp;
	
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
 * Erase columns.
 */
static void
rasops24_erasecols(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	struct rasops_info *ri;
	int32_t *dp;
	u_char *rp;
	int n8, n1, clr, height, cnt, slop1, slop2;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	/* Catches 'row < 0' case too */ 
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
		
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	num *= ri->ri_xscale;
	clr = rasops24_bg_color(ri, attr);
	height = ri->ri_font->fontheight;
	
	slop1 = (int)rp & 2;
	slop2 = (num - slop1) & 2;
	n8 = num >> 5;
	n1 = (num >> 2) & 7;

	while (height--) {
		cnt = num;
		dp = (u_int32_t *)rp;
		rp += ri->ri_stride;
	
		/* Align span to 4 bytes */
		if (slop1) {
			*(int16_t *)dp = (int16_t)clr;
			DELTA(dp, 2, int32_t *);
		}
	
		/* Write 32 bytes per loop */
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
		
		/* Write 4 bytes per loop */	
		for (cnt = n1; cnt; cnt--)
			*dp++ = clr;
	
		/* Write unaligned trailing slop */ 
		if (slop2) 
			*(int16_t *)dp = (int16_t)clr;
	}
}


/*
 * Paint a single character.
 */
static void
rasops24_putchar(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	u_char *dp, *rp;
	int32_t *fr, clr[16], fb;
	int width, height, cnt;
	
	if (uc == (u_int)-1) {
		rasops24_erasecols(cookie, row, col, 1, attr);
		return;
	}
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif
	
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	fr = (int32_t *)ri->ri_font->data + uc * ri->ri_font->fontheight;

	height = ri->ri_font->fontheight;
	width = ri->ri_font->fontwidth;
	
	clr[1] = ri->ri_devcmap[((u_int)attr >> 24)];
	clr[0] = ri->ri_devcmap[((u_int)attr >> 16)];

	while (height--) {
		dp = rp;
		fb = *fr++;
		rp += ri->ri_stride;
		
		for (cnt = width; cnt; cnt--) {
			*(int16_t *)dp = (int16_t)clr[fb & 1];
			fb >>= 1;
			dp += 2;
		}
	}	
}


/*
 * Construct a 12 byte by 1 byte blitting stamp. This uses the background
 * color only (for erasecols()/eraserows()).
 */
static void
rasops24_make_bgstamp(i, bp)
	int i;
	u_char *bp;
{
	u_char r, g, b;
	
	i = (i >> 8) & 0xff;
	i = (i << 1) + i;
	r = rasops_cmap[i+0];
	g = rasops_cmap[i+1];
	b = rasops_cmap[i+2];

#if BYTE_ORDER == LITTLE_ENDIAN
	bp[0] = r; bp[1] = g; bp[2] = b;  bp[3] = r;
	bp[4] = g; bp[5] = b; bp[6] = r;  bp[7] = g;
	bp[8] = b; bp[9] = r; bp[10] = g; bp[11] = b;
#else
	bp[3] = r;  bp[2] = g;  bp[1] = b; bp[0] = r;
	bp[7] = g;  bp[6] = b;  bp[5] = r; bp[4] = g;
	bp[11] = b; bp[10] = r; bp[9] = g; bp[8] = b;
#endif
}


/*
 * Erase rows. 
 */
static void
rasops24_eraserows(cookie, row, num, attr)
	void *cookie;
	int row, num;
	long attr;
{
	struct rasops_info *ri;
	int32_t *dp, clr, stamp[3];
	int n9, n3, n1, cnt;
	
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
	
	n9 = ri->ri_emustride / 36;
	cnt = (n9 << 5) + (n9 << 2); /* (32*n9) + (4*n9) */
	
	n3 = (ri->ri_emustride - cnt) / 12;
	cnt += (n3 << 3) + (n3 << 2); /* (8*n3) + (4*n3) */
	
	n1 = (ri->ri_emustride - cnt) >> 2;
	
	/* If the color is gray, we can cheat... */
	if (attr & ATTR_GRAY_BG) {
		clr = rasops24_bg_color(ri, attr);
	
		while (num--) {
			for (cnt = n9; cnt; cnt--) {
				dp[0] = clr;
				dp[1] = clr;
				dp[2] = clr;
				dp[3] = clr;
				dp[4] = clr;
				dp[5] = clr;
				dp[6] = clr;
				dp[7] = clr;
				dp[8] = clr;
				dp += 9;
			}

			for (cnt = n3; cnt; cnt--) {
				dp[0] = clr;
				dp[1] = clr;
				dp[2] = clr;
				dp += 3;
			}
			
			for (cnt = n1; cnt; cnt--)
				*dp++ = clr;
			
			DELTA(dp, ri->ri_delta, int32_t *);
		}
	} else {
		rasops24_make_bgstamp((int)attr, (u_char *)stamp);

		while (num--) {
			for (cnt = n9; cnt; cnt--) {
				dp[0] = stamp[0];
				dp[1] = stamp[1];
				dp[2] = stamp[2];
				dp[3] = stamp[0];
				dp[4] = stamp[1];
				dp[5] = stamp[2];
				dp[6] = stamp[0];
				dp[7] = stamp[1];
				dp[8] = stamp[2];
				dp += 9;
			}

			for (cnt = n3; cnt; cnt--) {
				dp[0] = stamp[0];
				dp[1] = stamp[1];
				dp[2] = stamp[2];
				dp += 3;
			}
			
			for (cnt = 0; cnt < n1; cnt++)
				*dp++ = stamp[cnt];
				
			DELTA(dp, ri->ri_delta, int32_t *);
		}
	}
}

#endif /* RASOPS24 */
