/* 	$NetBSD: rasops24.c,v 1.36 2019/07/28 12:06:10 rin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rasops24.c,v 1.36 2019/07/28 12:06:10 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <machine/endian.h>
#include <sys/bswap.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops24_erasecols(void *, int, int, int, long);
static void 	rasops24_eraserows(void *, int, int, long);
static void 	rasops24_putchar(void *, int, int, u_int, long);
#ifndef RASOPS_SMALL
static void 	rasops24_putchar8(void *, int, int, u_int, long);
static void 	rasops24_putchar12(void *, int, int, u_int, long);
static void 	rasops24_putchar16(void *, int, int, u_int, long);
static void	rasops24_makestamp(struct rasops_info *, long);

/*
 * 4x1 stamp for optimized character blitting
 */
static uint32_t	stamp[64];
static long	stamp_attr;
static int	stamp_mutex;	/* XXX see note in readme */
#endif

/*
 * offset = STAMP_SHIFT(fontbits, nibble #) & STAMP_MASK
 * destination uint32_t[0] = STAMP_READ(offset)
 * destination uint32_t[1] = STAMP_READ(offset + 4)
 * destination uint32_t[2] = STAMP_READ(offset + 8)
 */
#define	STAMP_SHIFT(fb, n)	((n) ? (fb) : (fb) << 4)
#define	STAMP_MASK		(0xf << 4)
#define	STAMP_READ(o)		(*(uint32_t *)((uint8_t *)stamp + (o)))

/*
 * Initialize rasops_info struct for this colordepth.
 */
void
rasops24_init(struct rasops_info *ri)
{

	if (ri->ri_rnum == 0) {
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 8;

		ri->ri_rpos = 0;
		ri->ri_gpos = 8;
		ri->ri_bpos = 16;
	}

	ri->ri_ops.erasecols = rasops24_erasecols;
	ri->ri_ops.eraserows = rasops24_eraserows;

	switch (ri->ri_font->fontwidth) {
#ifndef RASOPS_SMALL
	case 8:
		ri->ri_ops.putchar = rasops24_putchar8;
		break;
	case 12:
		ri->ri_ops.putchar = rasops24_putchar12;
		break;
	case 16:
		ri->ri_ops.putchar = rasops24_putchar16;
		break;
#endif
	default:
		ri->ri_ops.putchar = rasops24_putchar;
		break;
	}
}

#define	RASOPS_DEPTH	24
#include "rasops_putchar.h"

#ifndef RASOPS_SMALL
/*
 * Recompute the blitting stamp.
 */
static void
rasops24_makestamp(struct rasops_info *ri, long attr)
{
	uint32_t fg, bg, c1, c2, c3, c4;
	int i;

	fg = ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf] & 0xffffff;
	bg = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	stamp_attr = attr;

	for (i = 0; i < 64; i += 4) {
#if BYTE_ORDER == LITTLE_ENDIAN
		c1 = i & 32 ? fg : bg;
		c2 = i & 16 ? fg : bg;
		c3 = i &  8 ? fg : bg;
		c4 = i &  4 ? fg : bg;
#else
		c1 = i &  8 ? fg : bg;
		c2 = i &  4 ? fg : bg;
		c3 = i & 16 ? fg : bg;
		c4 = i & 32 ? fg : bg;
#endif
		stamp[i + 0] = (c1 <<  8) | (c2 >> 16);
		stamp[i + 1] = (c2 << 16) | (c3 >>  8);
		stamp[i + 2] = (c3 << 24) |  c4;

#if BYTE_ORDER == LITTLE_ENDIAN
		if ((ri->ri_flg & RI_BSWAP) == 0) {
#else
		if ((ri->ri_flg & RI_BSWAP) != 0) {
#endif
			stamp[i + 0] = bswap32(stamp[i + 0]);
			stamp[i + 1] = bswap32(stamp[i + 1]);
			stamp[i + 2] = bswap32(stamp[i + 2]);
		}
	}
}

#define	RASOPS_WIDTH	8
#include "rasops_putchar_width.h"
#undef	RASOPS_WIDTH

#define	RASOPS_WIDTH	12
#include "rasops_putchar_width.h"
#undef	RASOPS_WIDTH

#define	RASOPS_WIDTH	16
#include "rasops_putchar_width.h"
#undef	RASOPS_WIDTH

#endif	/* !RASOPS_SMALL */

/*
 * Erase rows. This is nice and easy due to alignment.
 */
static void
rasops24_eraserows(void *cookie, int row, int num, long attr)
{
	int n9, n3, n1, cnt, stride, delta;
	uint32_t *dp, clr, xstamp[3];
	struct rasops_info *ri;

	/*
	 * If the color is gray, we can cheat and use the generic routines
	 * (which are faster, hopefully) since the r,g,b values are the same.
	 */
	if ((attr & WSATTR_PRIVATE2) != 0) {
		rasops_eraserows(cookie, row, num, attr);
		return;
	}

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

	clr = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	xstamp[0] = (clr <<  8) | (clr >> 16);
	xstamp[1] = (clr << 16) | (clr >>  8);
	xstamp[2] = (clr << 24) | clr;

#if BYTE_ORDER == LITTLE_ENDIAN
	if ((ri->ri_flg & RI_BSWAP) == 0) {
#else
	if ((ri->ri_flg & RI_BSWAP) != 0) {
#endif
		xstamp[0] = bswap32(xstamp[0]);
		xstamp[1] = bswap32(xstamp[1]);
		xstamp[2] = bswap32(xstamp[2]);
	}

	/*
	 * XXX the wsdisplay_emulops interface seems a little deficient in
	 * that there is no way to clear the *entire* screen. We provide a
	 * workaround here: if the entire console area is being cleared, and
	 * the RI_FULLCLEAR flag is set, clear the entire display.
	 */
	if (num == ri->ri_rows && (ri->ri_flg & RI_FULLCLEAR) != 0) {
		stride = ri->ri_stride;
		num = ri->ri_height;
		dp = (uint32_t *)ri->ri_origbits;
		delta = 0;
	} else {
		stride = ri->ri_emustride;
		num *= ri->ri_font->fontheight;
		dp = (uint32_t *)(ri->ri_bits + row * ri->ri_yscale);
		delta = ri->ri_delta;
	}

	n9 = stride / 36;
	cnt = (n9 << 5) + (n9 << 2); /* (32*n9) + (4*n9) */
	n3 = (stride - cnt) / 12;
	cnt += (n3 << 3) + (n3 << 2); /* (8*n3) + (4*n3) */
	n1 = (stride - cnt) >> 2;

	while (num--) {
		for (cnt = n9; cnt; cnt--) {
			dp[0] = xstamp[0];
			dp[1] = xstamp[1];
			dp[2] = xstamp[2];
			dp[3] = xstamp[0];
			dp[4] = xstamp[1];
			dp[5] = xstamp[2];
			dp[6] = xstamp[0];
			dp[7] = xstamp[1];
			dp[8] = xstamp[2];
			dp += 9;
		}

		for (cnt = n3; cnt; cnt--) {
			dp[0] = xstamp[0];
			dp[1] = xstamp[1];
			dp[2] = xstamp[2];
			dp += 3;
		}

		for (cnt = 0; cnt < n1; cnt++)
			*dp++ = xstamp[cnt];

		DELTA(dp, delta, uint32_t *);
	}
}

/*
 * Erase columns.
 */
static void
rasops24_erasecols(void *cookie, int row, int col, int num, long attr)
{
	int n12, n4, height, cnt, slop, clr, xstamp[3];
	struct rasops_info *ri;
	uint32_t *dp, *rp;
	uint8_t *dbp;

	/*
	 * If the color is gray, we can cheat and use the generic routines
	 * (which are faster, hopefully) since the r,g,b values are the same.
	 */
	if ((attr & WSATTR_PRIVATE2) != 0) {
		rasops_erasecols(cookie, row, col, num, attr);
		return;
	}

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

	rp = (uint32_t *)(ri->ri_bits + row*ri->ri_yscale + col*ri->ri_xscale);
	num *= ri->ri_font->fontwidth;
	height = ri->ri_font->fontheight;

	clr = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	xstamp[0] = (clr <<  8) | (clr >> 16);
	xstamp[1] = (clr << 16) | (clr >>  8);
	xstamp[2] = (clr << 24) | clr;

#if BYTE_ORDER == LITTLE_ENDIAN
	if ((ri->ri_flg & RI_BSWAP) == 0) {
#else
	if ((ri->ri_flg & RI_BSWAP) != 0) {
#endif
		xstamp[0] = bswap32(xstamp[0]);
		xstamp[1] = bswap32(xstamp[1]);
		xstamp[2] = bswap32(xstamp[2]);
	}

	/*
	 * The current byte offset mod 4 tells us the number of 24-bit pels
	 * we need to write for alignment to 32-bits. Once we're aligned on
	 * a 32-bit boundary, we're also aligned on a 4 pixel boundary, so
	 * the stamp does not need to be rotated. The following shows the
	 * layout of 4 pels in a 3 word region and illustrates this:
	 *
	 *	aaab bbcc cddd
	 */
	slop = (int)(long)rp & 3;	num -= slop;
	n12 = num / 12;		num -= (n12 << 3) + (n12 << 2);
	n4 = num >> 2;		num &= 3;

	while (height--) {
		dbp = (uint8_t *)rp;
		DELTA(rp, ri->ri_stride, uint32_t *);

		/* Align to 4 bytes */
		/* XXX handle with masks, bring under control of RI_BSWAP */
		for (cnt = slop; cnt; cnt--) {
			*dbp++ = (clr >> 16);
			*dbp++ = (clr >> 8);
			*dbp++ = clr;
		}

		dp = (uint32_t *)dbp;

		/* 12 pels per loop */
		for (cnt = n12; cnt; cnt--) {
			dp[0] = xstamp[0];
			dp[1] = xstamp[1];
			dp[2] = xstamp[2];
			dp[3] = xstamp[0];
			dp[4] = xstamp[1];
			dp[5] = xstamp[2];
			dp[6] = xstamp[0];
			dp[7] = xstamp[1];
			dp[8] = xstamp[2];
			dp += 9;
		}

		/* 4 pels per loop */
		for (cnt = n4; cnt; cnt--) {
			dp[0] = xstamp[0];
			dp[1] = xstamp[1];
			dp[2] = xstamp[2];
			dp += 3;
		}

		/* Trailing slop */
		/* XXX handle with masks, bring under control of RI_BSWAP */
		dbp = (uint8_t *)dp;
		for (cnt = num; cnt; cnt--) {
			*dbp++ = (clr >> 16);
			*dbp++ = (clr >> 8);
			*dbp++ = clr;
		}
	}
}
