/* $NetBSD: rasops32.c,v 1.1 1999/04/13 00:18:01 ad Exp $ */

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
#ifdef RASOPS32

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops32.c,v 1.1 1999/04/13 00:18:01 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops32_putchar __P((void *, int, int, u_int, long attr));
static void 	rasops32_erasecols __P((void *, int, int, int, long));
static void 	rasops32_eraserows __P((void *, int, int, long));
static int32_t	rasops32_fg_color __P((struct rasops_info *, long));
static int32_t	rasops32_bg_color __P((struct rasops_info *, long));
static void	rasops32_do_cursor __P((struct rasops_info *));
void		rasops32_init __P((struct rasops_info *ri));

/*
 * Initalize a 'rasops_info' descriptor for this depth.
 */
void
rasops32_init(ri)
	struct rasops_info *ri;
{
	
	/* 
	 * This sucks. There is little optimization you can do with this 
	 * colordepth on 32-bit machines.
  	 *
	 * XXX c'mon sparc, alpha ppl?
	 */
	ri->ri_ops.putchar = rasops32_putchar;
	ri->ri_ops.erasecols = rasops32_erasecols;
	ri->ri_ops.eraserows = rasops32_eraserows;		
	ri->ri_do_cursor = rasops32_do_cursor;
	rasops_init_devcmap(ri);
}


/*
 * Get background color from attribute.
 */
static __inline__ int32_t
rasops32_bg_color(ri, attr)
	struct rasops_info *ri;
	long attr;
{
	
	return ri->ri_devcmap[((int32_t)attr >> 24) & 15];
}


/*
 * Get foreground color from attribute.
 */
static __inline__ int32_t
rasops32_fg_color(ri, attr)
	struct rasops_info *ri;
	long attr;
{
	
	return ri->ri_devcmap[((int32_t)attr >> 16) & 15];
}


/*
 * Actually turn the cursor on or off. This does the dirty work for
 * rasops_cursor().
 */
static void
rasops32_do_cursor(ri)
	struct rasops_info *ri;
{
	u_char *rp;
	int32_t *dp, planemask;
	int num, height, cnt, row, col;
	
	row = ri->ri_crow;
	col = ri->ri_ccol;
	
	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	height = ri->ri_font->fontheight;
	num = ri->ri_xscale >> 2;
	planemask = ri->ri_devcmap[15];

	while (height--) {
		dp = (int32_t *)rp;
		rp += ri->ri_stride;
	
		for (cnt = num; cnt; cnt--)
			*dp++ ^= planemask;
	}
}


/*
 * Paint a single character.
 */
static void
rasops32_putchar(cookie, row, col, uc, attr)
	void *cookie;
	int row, col;
	u_int uc;
	long attr;
{
	struct rasops_info *ri;
	int32_t *dp, *rp, clr[2];
	u_char *fr;
	int width, height, cnt, fs, fb;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING	
	/* Catches 'row < 0' case too */ 
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if ((unsigned)col >= (unsigned)ri->ri_cols)
		return;
#endif
	
	rp = (int32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);

	height = ri->ri_font->fontheight;
	width = ri->ri_font->fontwidth;
	clr[0] = rasops32_bg_color(ri, attr);
	clr[1] = rasops32_fg_color(ri, attr);
		
	if (uc == ' ') {
		while (height--) {
			dp = rp;
			DELTA(rp, ri->ri_stride, int32_t *);

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
			DELTA(rp, ri->ri_stride, int32_t *);
			
			for (cnt = width; cnt; cnt--) {
				*dp++ = clr[(fb >> 31) & 1];
				fb <<= 1;
			}
		}
	}	

	/* Do underline */
	if (attr & 1) {
		DELTA(rp, -(ri->ri_stride << 1), int32_t *);

		while (width--)
			*rp++ = clr[1];
	}	
}


/*
 * Erase rows. 
 */
static void
rasops32_eraserows(cookie, row, num, attr)
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
	clr = rasops32_bg_color(ri, attr);

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
rasops32_erasecols(cookie, row, col, num, attr)
	void *cookie;
	int row, col, num;
	long attr;
{
	int n8, clr, height, cnt;
	struct rasops_info *ri;
	int32_t *dst;
	u_char *rp;
	
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
	clr = rasops32_bg_color(ri, attr);
	height = ri->ri_font->fontheight;
	n8 = num >> 5;
	num = (num >> 2) & 7;

	while (height--) {
		dst = (int32_t *)rp;
		rp += ri->ri_stride;
	
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
		
		for (cnt = num; cnt; cnt--)
			*dst++ = clr;
	}
}

#endif /* RASOPS32 */
