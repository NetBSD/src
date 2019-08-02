/* 	$NetBSD: rasops24.c,v 1.45 2019/08/02 23:05:42 rin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rasops24.c,v 1.45 2019/08/02 23:05:42 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <machine/endian.h>
#include <sys/bswap.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#define	_RASOPS_PRIVATE
#define	RASOPS_DEPTH	24
#include <dev/rasops/rasops.h>

static void 	rasops24_erasecols(void *, int, int, int, long);
static void 	rasops24_eraserows(void *, int, int, long);
static void 	rasops24_putchar(void *, int, int, u_int, long);
static void 	rasops24_putchar_aa(void *, int, int, u_int, long);
static __inline void
		rasops24_makestamp1(struct rasops_info *, uint32_t *,
				    uint32_t, uint32_t, uint32_t, uint32_t);
#ifndef RASOPS_SMALL
static void 	rasops24_putchar8(void *, int, int, u_int, long);
static void 	rasops24_putchar12(void *, int, int, u_int, long);
static void 	rasops24_putchar16(void *, int, int, u_int, long);
static void	rasops24_makestamp(struct rasops_info *, long);
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

	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = rasops24_putchar_aa;
		return;
	}

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
		return;
	}

#ifndef RASOPS_SMALL
	rasops_allocstamp(ri, sizeof(uint32_t) * 64);
#endif
}

#include "rasops_putchar.h"
#include "rasops_putchar_aa.h"

static __inline void
rasops24_makestamp1(struct rasops_info *ri, uint32_t *stamp,
    uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4)
{

	stamp[0] = (c1 <<  8) | (c2 >> 16);
	stamp[1] = (c2 << 16) | (c3 >>  8);
	stamp[2] = (c3 << 24) |  c4;

#if BYTE_ORDER == LITTLE_ENDIAN
	if ((ri->ri_flg & RI_BSWAP) == 0)
#else
	if ((ri->ri_flg & RI_BSWAP) != 0)
#endif
	{
		stamp[0] = bswap32(stamp[0]);
		stamp[1] = bswap32(stamp[1]);
		stamp[2] = bswap32(stamp[2]);
	}
}

#ifndef RASOPS_SMALL
/*
 * Recompute the blitting stamp.
 */
static void
rasops24_makestamp(struct rasops_info *ri, long attr)
{
	uint32_t *stamp = (uint32_t *)ri->ri_stamp;
	uint32_t fg, bg, c1, c2, c3, c4;
	int i;

	fg = ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf] & 0xffffff;
	bg = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	ri->ri_stamp_attr = attr;

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
		rasops24_makestamp1(ri, &stamp[i], c1, c2, c3, c4);
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
	struct rasops_info *ri = (struct rasops_info *)cookie;
	uint32_t *buf = (uint32_t *)ri->ri_buf;
	int full, slop, cnt, stride;
	uint32_t *rp, *dp, *hp, clr, stamp[3];

	hp = NULL;	/* XXX GCC */

	/*
	 * If the color is gray, we can cheat and use the generic routines
	 * (which are faster, hopefully) since the r,g,b values are the same.
	 */
	if ((attr & WSATTR_PRIVATE2) != 0) {
		rasops_eraserows(cookie, row, num, attr);
		return;
	}

#ifdef RASOPS_CLIPPING
	if (row < 0) {
		num += row;
		row = 0;
	}

	if (row + num > ri->ri_rows)
		num = ri->ri_rows - row;

	if (num <= 0)
		return;
#endif

	clr = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	rasops24_makestamp1(ri, stamp, clr, clr, clr, clr);

	/*
	 * XXX the wsdisplay_emulops interface seems a little deficient in
	 * that there is no way to clear the *entire* screen. We provide a
	 * workaround here: if the entire console area is being cleared, and
	 * the RI_FULLCLEAR flag is set, clear the entire display.
	 */
	if (num == ri->ri_rows && (ri->ri_flg & RI_FULLCLEAR) != 0) {
		stride = ri->ri_stride;
		num = ri->ri_height;
		rp = (uint32_t *)ri->ri_origbits;
		if (ri->ri_hwbits)
			hp = (uint32_t *)ri->ri_hworigbits;
	} else {
		stride = ri->ri_emustride;
		num *= ri->ri_font->fontheight;
		rp = (uint32_t *)(ri->ri_bits + row * ri->ri_yscale);
		if (ri->ri_hwbits)
			hp = (uint32_t *)(ri->ri_hwbits + row * ri->ri_yscale);
	}

	full = stride / (4 * 3);
	slop = (stride - full * (4 * 3)) / 4;

	dp = buf;

	for (cnt = full; cnt; cnt--) {
		dp[0] = stamp[0];
		dp[1] = stamp[1];
		dp[2] = stamp[2];
		dp += 3;
	}

	for (cnt = 0; cnt < slop; cnt++)
		*dp++ = stamp[cnt];

	while (num--) {
		memcpy(rp, buf, stride);
		DELTA(rp, ri->ri_stride, uint32_t *);
		if (ri->ri_hwbits) {
			memcpy(hp, buf, stride);
			DELTA(hp, ri->ri_stride, uint32_t *);
		}
	}
}

/*
 * Erase columns.
 */
static void
rasops24_erasecols(void *cookie, int row, int col, int num, long attr)
{
	struct rasops_info *ri = (struct rasops_info *)cookie;
	void *buf = ri->ri_buf;
	int height, cnt, clr, stamp[3];
	uint32_t *dp;
	uint8_t *rp, *hp, *dbp;

	hp = NULL;	/* XXX GCC */

	/*
	 * If the color is gray, we can cheat and use the generic routines
	 * (which are faster, hopefully) since the r,g,b values are the same.
	 */
	if ((attr & WSATTR_PRIVATE2) != 0) {
		rasops_erasecols(cookie, row, col, num, attr);
		return;
	}

#ifdef RASOPS_CLIPPING
	/* Catches 'row < 0' case too */
	if ((unsigned)row >= (unsigned)ri->ri_rows)
		return;

	if (col < 0) {
		num += col;
		col = 0;
	}

	if (col + num > ri->ri_cols)
		num = ri->ri_cols - col;

	if (num <= 0)
		return;
#endif

	rp = ri->ri_bits + row * ri->ri_yscale + col * ri->ri_xscale;
	if (ri->ri_hwbits)
		hp = ri->ri_hwbits + row * ri->ri_yscale + col * ri->ri_xscale;

	num *= ri->ri_font->fontwidth;
	height = ri->ri_font->fontheight;

	clr = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffffff;
	rasops24_makestamp1(ri, stamp, clr, clr, clr, clr);

	/* 4 pels per loop */
	dp = (uint32_t *)buf;
	for (cnt = num >> 2; cnt; cnt--) {
		dp[0] = stamp[0];
		dp[1] = stamp[1];
		dp[2] = stamp[2];
		dp += 3;
	}

	/* Trailing slop */
	/* XXX handle with masks, bring under control of RI_BSWAP */
	dbp = (uint8_t *)dp;
	for (cnt = num & 3; cnt; cnt--) {
		*dbp++ = (clr >> 16);
		*dbp++ = (clr >> 8);
		*dbp++ =  clr;
	}

	num *= 3;

	while (height--) {
		memcpy(rp, buf, num);
		rp += ri->ri_stride;
		if (ri->ri_hwbits) {
			memcpy(hp, buf, num);
			hp += ri->ri_stride;
		}
	}
}
