/* $NetBSD: rasops.c,v 1.1 1999/04/13 00:17:58 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: rasops.c,v 1.1 1999/04/13 00:17:58 ad Exp $");

#include "opt_rasops.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

/* ANSI colormap (R,G,B). Upper 8 are high-intensity */
u_char rasops_cmap[256*3] = {
	0x00, 0x00, 0x00, /* black */
	0x7f, 0x00, 0x00, /* red */
	0x00, 0x7f, 0x00, /* green */
	0x7f, 0x7f, 0x00, /* brown */
	0x00, 0x00, 0x7f, /* blue */
	0x7f, 0x00, 0x7f, /* magenta */
	0x00, 0x7f, 0x7f, /* cyan */
	0xc7, 0xc7, 0xc7, /* white - XXX too dim? */

	0x7f, 0x7f, 0x7f, /* black */
	0xff, 0x00, 0x00, /* red */
	0x00, 0xff, 0x00, /* green */
	0xff, 0xff, 0x00, /* brown */
	0x00, 0x00, 0xff, /* blue */
	0xff, 0x00, 0xff, /* magenta */
	0x00, 0xff, 0xff, /* cyan */
	0xff, 0xff, 0xff, /* white */
};

/* True if color is gray */
u_char rasops_isgray[16] = { 
	1, 0, 0, 0, 
	0, 0, 0, 1,
	1, 0, 0, 0,
	0, 0, 0, 1
};

/* Common functions */
static void	rasops_copycols __P((void *, int, int, int, int));
static void	rasops_copyrows __P((void *, int, int, int));
static int	rasops_mapchar __P((void *, int, u_int *));
static void	rasops_cursor __P((void *, int, int, int));
static int	rasops_alloc_cattr __P((void *, int, int, int, long *));
static int	rasops_alloc_mattr __P((void *, int, int, int, long *));

/* Per-depth initalization functions */
void	rasops1_init __P((struct rasops_info *));
void	rasops8_init __P((struct rasops_info *));
void	rasops15_init __P((struct rasops_info *));
void	rasops24_init __P((struct rasops_info *));
void	rasops32_init __P((struct rasops_info *));

/*
 * Initalize a 'rasops_info' descriptor.
 */
int
rasops_init(ri, wantrows, wantcols, clear, center)
	struct rasops_info *ri;
	int wantrows, wantcols, clear, center;
{
	
	/* Select a font if the caller doesn't care */
	if (ri->ri_font == NULL) {
		int cookie;
		
		wsfont_init();

		/* Want 8 pixel wide, don't care about aestethics */
		if ((cookie = wsfont_find(NULL, 8, 0, 0)) < 0)
			cookie = wsfont_find(NULL, 0, 0, 0);

		if (cookie < 0) {
			printf("rasops_init: font table is empty\n");
			return (-1);
		}
		
		if (wsfont_lock(cookie, &ri->ri_font, 
		    WSFONT_LITTLE, WSFONT_LITTLE) < 0) {
			printf("rasops_init: couldn't lock font\n");
			return (-1);
		}
	}
	
	/* This should never happen in reality... */
#ifdef DEBUG
	if ((int)ri->ri_bits & 3) {
		printf("rasops_init: bits not aligned on 32-bit boundary\n");
		return (-1);
	}

	if ((int)ri->ri_stride & 3) {
		printf("rasops_init: stride not aligned on 32-bit boundary\n");
		return (-1);
	}
		
	if (ri->ri_font->fontwidth > 32) {
		printf("rasops_init: fontwidth > 32\n");
		return (-1);
	}
#endif
	
	/* Fix color palette. We need this for the cursor to work. */
	rasops_cmap[255*3+0] = 0xff;
	rasops_cmap[255*3+1] = 0xff;
	rasops_cmap[255*3+2] = 0xff;
	
	/* Don't care if the caller wants a hideously small console */
	if (wantrows < 10)
		wantrows = 5000;
		
	if (wantcols < 20)
		wantcols = 5000;
	
	/* Now constrain what they get */
	ri->ri_emuwidth = ri->ri_font->fontwidth * wantcols;
	ri->ri_emuheight = ri->ri_font->fontheight * wantrows;
	
	if (ri->ri_emuwidth > ri->ri_width)
		ri->ri_emuwidth = ri->ri_width;
		
	if (ri->ri_emuheight > ri->ri_height)
		ri->ri_emuheight = ri->ri_height;
	
	/* Reduce width until aligned on a 32-bit boundary */
	while ((ri->ri_emuwidth*ri->ri_depth & 31) != 0)
		ri->ri_emuwidth--;
	
	ri->ri_cols = ri->ri_emuwidth / ri->ri_font->fontwidth;
	ri->ri_rows = ri->ri_emuheight / ri->ri_font->fontheight;
	ri->ri_emustride = ri->ri_emuwidth * ri->ri_depth >> 3;
	ri->ri_delta = ri->ri_stride - ri->ri_emustride;
	ri->ri_ccol = 0;
	ri->ri_crow = 0;
	ri->ri_pelbytes = ri->ri_depth >> 3;
	
	ri->ri_xscale = (ri->ri_font->fontwidth * ri->ri_depth) >> 3;
	ri->ri_yscale = ri->ri_font->fontheight * ri->ri_stride;
	ri->ri_fontscale = ri->ri_font->fontheight * ri->ri_font->stride;

#ifdef DEBUG	
	if (ri->ri_delta & 3)
		panic("rasops_init: delta isn't aligned on 32-bit boundary!");
#endif	
	/* Clear the entire display */
	if (clear)
		bzero(ri->ri_bits, ri->ri_stride * ri->ri_height);
	
	/* Now centre our window if needs be */ 
	ri->ri_origbits = ri->ri_bits;
	
	if (center) {
		ri->ri_bits += ((ri->ri_stride - ri->ri_emustride) >> 1) & ~3;
		ri->ri_bits += ((ri->ri_height - ri->ri_emuheight) >> 1) * 
		    ri->ri_stride;
	}
	
	/* Fill in defaults for operations set */
	ri->ri_ops.mapchar = rasops_mapchar;
	ri->ri_ops.copyrows = rasops_copyrows;
	ri->ri_ops.copycols = rasops_copycols;
	ri->ri_ops.cursor = rasops_cursor;
	
	if (ri->ri_depth == 1 || ri->ri_forcemono)
		ri->ri_ops.alloc_attr = rasops_alloc_mattr;
	else
		ri->ri_ops.alloc_attr = rasops_alloc_cattr;
			
	switch (ri->ri_depth) {
#ifdef RASOPS1
	case 1:
		rasops1_init(ri);
		break;
#endif

#ifdef RASOPS8
	case 8:
		rasops8_init(ri);
		break;
#endif

#if defined(RASOPS15) || defined(RASOPS16)
	case 15:
	case 16:
		rasops15_init(ri);
		break;
#endif

#ifdef RASOPS24
	case 24:
		rasops24_init(ri);
		break;
#endif

#ifdef RASOPS24
	case 32:
		rasops32_init(ri);
		break;
#endif
	default:
		ri->ri_flg = 0;
		return (-1);
	}
	
	ri->ri_flg = RASOPS_INITTED;	
	return (0);
}


/*
 * Map a character.
 */
static int
rasops_mapchar(cookie, c, cp)
	void *cookie;
	int c;
	u_int *cp;
{
	struct rasops_info *ri;
	
	ri = (struct rasops_info *)cookie;
	
	if (c < ri->ri_font->firstchar) {
		*cp = ' ';
		return (0);
	}
	
	if (c - ri->ri_font->firstchar >= ri->ri_font->numchars) {
		*cp = ' ';
		return (0);
	}
	
	*cp = c;
	return (5);
}


/*
 * Allocate a color attribute.
 */
static int
rasops_alloc_cattr(cookie, fg, bg, flg, attr)
	void *cookie;
	int fg, bg, flg;
	long *attr;
{
	int swap;

#ifdef RASOPS_CLIPPING
	fg &= 7;
	bg &= 7;
	flg &= 255;
#endif
	if (flg & WSATTR_BLINK)
		return (EINVAL);
		
	if (flg & WSATTR_REVERSE) {
		swap = fg;
		fg = bg;
		bg = swap;
	}

	if (flg & WSATTR_HILIT)
		fg += 8;
		
	flg = ((flg & WSATTR_UNDERLINE) ? 1 : 0);
	
	if (rasops_isgray[fg])
		flg |= 2;

	if (rasops_isgray[bg])
		flg |= 4;

	*attr = (bg << 16) | (fg << 24) | flg;
	return 0;
}


/*
 * Allocate a mono attribute.
 */
static int
rasops_alloc_mattr(cookie, fg, bg, flg, attr)
	void *cookie;
	int fg, bg, flg;
	long *attr;
{
	int swap;

#ifdef RASOPS_CLIPPING
	flg &= 255;
#endif
	fg = fg ? 1 : 0;
	bg = bg ? 1 : 0;
	
	if (flg & WSATTR_BLINK)
		return (EINVAL);
		
	if (!(flg & WSATTR_REVERSE) ^ !(flg & WSATTR_HILIT)) {
		swap = fg;
		fg = bg;
		bg = swap;
	}

	*attr = (bg << 16) | (fg << 24) | ((flg & WSATTR_UNDERLINE) ? 7 : 6);
	return 0;
}


/*
 * Copy rows.
 */
static void
rasops_copyrows(cookie, src, dst, num)
	void *cookie;
	int src, dst, num;
{
	struct rasops_info *ri;
	int32_t *sp, *dp, *srp, *drp;
	int n8, n1, cnt;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING
	if (dst == src)
		return;
	
	if (src < 0) {
		num += src;
		src = 0;
	}

	if ((src + num) > ri->ri_rows)
		num = ri->ri_rows - src;

	if (dst < 0) {
		num += dst;
		dst = 0;
	}

	if ((dst + num) > ri->ri_rows)
		num = ri->ri_rows - dst;
	
	if (num <= 0)
		return;
#endif

	num *= ri->ri_font->fontheight;
	n8 = ri->ri_emustride >> 5;
	n1 = (ri->ri_emustride >> 2) & 7;
	
	if (dst < src) {
		sp = (int32_t *)(ri->ri_bits + src * ri->ri_yscale);
		dp = (int32_t *)(ri->ri_bits + dst * ri->ri_yscale);
	
		while (num--) {
			for (cnt = n8; cnt; cnt--) {
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
				dp[3] = sp[3];
				dp[4] = sp[4];
				dp[5] = sp[5];
				dp[6] = sp[6];
				dp[7] = sp[7];
				dp += 8;
				sp += 8;
			}
			
			for (cnt = n1; cnt; cnt--)
				*dp++ = *sp++;
				
			DELTA(dp, ri->ri_delta, int32_t *);
			DELTA(sp, ri->ri_delta, int32_t *);
		}
	} else {
		src = ri->ri_font->fontheight * src + num - 1;
		dst = ri->ri_font->fontheight * dst + num - 1;
		
		srp = (int32_t *)(ri->ri_bits + src * ri->ri_stride);
		drp = (int32_t *)(ri->ri_bits + dst * ri->ri_stride);

		while (num--) {
			dp = drp;
			sp = srp;
			DELTA(srp, -ri->ri_stride, int32_t *);
			DELTA(drp, -ri->ri_stride, int32_t *);
		
			for (cnt = n8; cnt; cnt--) {
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
				dp[3] = sp[3];
				dp[4] = sp[4];
				dp[5] = sp[5];
				dp[6] = sp[6];
				dp[7] = sp[7];
				dp += 8;
				sp += 8;
			}
			
			for (cnt = n1; cnt; cnt--)
				*dp++ = *sp++;
		}
	}
}


/*
 * Copy columns. This is slow, and hard to optimize due to alignment,
 * and the fact that we have to copy both left->right and right->left.
 * We simply cop-out here and use bcopy(), since it handles all of
 * these cases anyway.
 */
static void
rasops_copycols(cookie, row, src, dst, num)
	void *cookie;
	int row, src, dst, num;
{
	struct rasops_info *ri;
	u_char *sp, *dp;
	int height;
	
	ri = (struct rasops_info *)cookie;

#ifdef RASOPS_CLIPPING
	if (dst == src)
		return;
		
	if (row < 0 || row >= ri->ri_rows)
		return;
	
	if (src < 0) {
		num += src;
		src = 0;
	}

	if ((src + num) > ri->ri_cols)
		num = ri->ri_cols - src;

	if (dst < 0) {
		num += dst;
		dst = 0;
	}

	if ((dst + num) > ri->ri_cols)
		num = ri->ri_cols - dst;
	
	if (num <= 0)
		return;
#endif
	
	num *= ri->ri_xscale;
	row *= ri->ri_yscale;
	height = ri->ri_font->fontheight;
	
	sp = ri->ri_bits + row + src * ri->ri_xscale;
	dp = ri->ri_bits + row + dst * ri->ri_xscale;
	
	while (height--) {
		bcopy(sp, dp, num);
		dp += ri->ri_stride;
		sp += ri->ri_stride;
	}
}


/*
 * Turn cursor off/on.
 */
static void
rasops_cursor(cookie, on, row, col)
	void *cookie;
	int on, row, col;
{
	struct rasops_info *ri;
	
	ri = (struct rasops_info *)cookie;
	
	/* Turn old cursor off */
	if (ri->ri_flg & RASOPS_CURSOR)
#ifdef RASOPS_CLIPPING		
		if (!(ri->ri_flg & RASOPS_CURSOR_CLIPPED))
#endif
			ri->ri_do_cursor(ri);

	/* Select new cursor */
#ifdef RASOPS_CLIPPING
	ri->ri_flg &= ~RASOPS_CURSOR_CLIPPED;
	
	if (row < 0 || row >= ri->ri_rows)
		ri->ri_flg |= RASOPS_CURSOR_CLIPPED;
	else if (col < 0 || col >= ri->ri_cols)
		ri->ri_flg |= RASOPS_CURSOR_CLIPPED;
#endif
	ri->ri_crow = row;
	ri->ri_ccol = col;

	if (on) {
		ri->ri_flg |= RASOPS_CURSOR;
#ifdef RASOPS_CLIPPING		
		if (!(ri->ri_flg & RASOPS_CURSOR_CLIPPED))
#endif
			ri->ri_do_cursor(ri);
	} else 
		ri->ri_flg &= ~RASOPS_CURSOR;
}


#if (RASOPS15 + RASOPS16 + RASOPS32)
/*
 * Make the device colormap
 */
void
rasops_init_devcmap(ri)
	struct rasops_info *ri;
{
	int i, c;
	u_char *p;
	
	p = rasops_cmap;
	
	for (i = 0; i < 16; i++) {
		if (ri->ri_rnum <= 8)
			c = (*p++ >> (8 - ri->ri_rnum)) << ri->ri_rpos;
		else 
			c = (*p++ << (ri->ri_rnum - 8)) << ri->ri_rpos;

		if (ri->ri_gnum <= 8)
			c = (*p++ >> (8 - ri->ri_gnum)) << ri->ri_gpos;
		else 
			c = (*p++ << (ri->ri_gnum - 8)) << ri->ri_gpos;

		if (ri->ri_bnum <= 8)
			c = (*p++ >> (8 - ri->ri_bnum)) << ri->ri_bpos;
		else 
			c = (*p++ << (ri->ri_bnum - 8)) << ri->ri_bpos;

		ri->ri_devcmap[i] = c;
	}
}
#endif


/*
 * Unpack a rasops attribute
 */
void
rasops_unpack_attr(attr, fg, bg, underline)
	long attr;
	int *fg, *bg, *underline;
{
	
	*fg = ((u_int)attr >> 24) & 15;
	*bg = ((u_int)attr >> 16) & 15;
	*underline = (u_int)attr & 1;
}
