/* 	$NetBSD: rasops2.c,v 1.27 2019/07/31 00:14:25 rin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rasops2.c,v 1.27 2019/07/31 00:14:25 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <machine/endian.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#define	_RASOPS_PRIVATE
#include <dev/rasops/rasops.h>
#include <dev/rasops/rasops_masks.h>

static void	rasops2_copycols(void *, int, int, int, int);
static void	rasops2_erasecols(void *, int, int, int, long);
static void	rasops2_do_cursor(struct rasops_info *);
static void	rasops2_putchar(void *, int, int col, u_int, long);
#ifndef RASOPS_SMALL
static void	rasops2_putchar8(void *, int, int col, u_int, long);
static void	rasops2_putchar12(void *, int, int col, u_int, long);
static void	rasops2_putchar16(void *, int, int col, u_int, long);
static void	rasops2_makestamp(struct rasops_info *, long);

/*
 * 4x1 stamp for optimized character blitting
 */
static uint8_t	stamp[16];
static long	stamp_attr;
static int	stamp_mutex;	/* XXX see note in README */
#endif

/*
 * offset = STAMP_SHIFT(fontbits, nibble #) & STAMP_MASK
 * destination = STAMP_READ(offset)
 */
#define	STAMP_SHIFT(fb, n)	((n) ? (fb) >> 4 : (fb))
#define	STAMP_MASK		0xf
#define	STAMP_READ(o)		stamp[o]

/*
 * Initialize rasops_info struct for this colordepth.
 */
void
rasops2_init(struct rasops_info *ri)
{

	switch (ri->ri_font->fontwidth) {
#ifndef RASOPS_SMALL
	case 8:
		ri->ri_ops.putchar = rasops2_putchar8;
		break;
	case 12:
		ri->ri_ops.putchar = rasops2_putchar12;
		break;
	case 16:
		ri->ri_ops.putchar = rasops2_putchar16;
		break;
#endif	/* !RASOPS_SMALL */
	default:
		panic("fontwidth not 8/12/16 or RASOPS_SMALL - fixme!");
		ri->ri_ops.putchar = rasops2_putchar;
		break;
	}

	if ((ri->ri_font->fontwidth & 3) != 0) {
		ri->ri_ops.erasecols = rasops2_erasecols;
		ri->ri_ops.copycols = rasops2_copycols;
		ri->ri_do_cursor = rasops2_do_cursor;
	}
}

/*
 * Put a single character. This is the generic version.
 */
static void
rasops2_putchar(void *cookie, int row, int col, u_int uc, long attr)
{

	/* XXX punt */
}

#ifndef RASOPS_SMALL
/*
 * Recompute the blitting stamp.
 */
static void
rasops2_makestamp(struct rasops_info *ri, long attr)
{
	int i, fg, bg;

	fg = ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf] & 3;
	bg = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 3;
	stamp_attr = attr;

	for (i = 0; i < 16; i++) {
#if BYTE_ORDER == BIG_ENDIAN
#define NEED_LITTLE_ENDIAN_STAMP RI_BSWAP
#else
#define NEED_LITTLE_ENDIAN_STAMP 0
#endif
		if ((ri->ri_flg & RI_BSWAP) == NEED_LITTLE_ENDIAN_STAMP) {
			/* littel endian */
			stamp[i]  = (i & 8 ? fg : bg);
			stamp[i] |= (i & 4 ? fg : bg) << 2;
			stamp[i] |= (i & 2 ? fg : bg) << 4;
			stamp[i] |= (i & 1 ? fg : bg) << 6;
		} else {
			/* big endian */
			stamp[i]  = (i & 1 ? fg : bg);
			stamp[i] |= (i & 2 ? fg : bg) << 2;
			stamp[i] |= (i & 4 ? fg : bg) << 4;
			stamp[i] |= (i & 8 ? fg : bg) << 6;
		}
	}
}

#define	RASOPS_DEPTH	2

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
 * Grab routines common to depths where (bpp < 8)
 */
#define NAME(ident)	rasops2_##ident
#define PIXEL_SHIFT	1

#include <dev/rasops/rasops_bitops.h>
