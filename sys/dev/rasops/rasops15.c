/* 	$NetBSD: rasops15.c,v 1.31 2019/07/29 10:55:56 rin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rasops15.c,v 1.31 2019/07/29 10:55:56 rin Exp $");

#include "opt_rasops.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

static void 	rasops15_putchar(void *, int, int, u_int, long);
static void 	rasops15_putchar_aa(void *, int, int, u_int, long);
#ifndef RASOPS_SMALL
static void 	rasops15_putchar8(void *, int, int, u_int, long);
static void 	rasops15_putchar12(void *, int, int, u_int, long);
static void 	rasops15_putchar16(void *, int, int, u_int, long);
static void	rasops15_makestamp(struct rasops_info *, long);
#endif

#ifndef RASOPS_SMALL
/*
 * 4x1 stamp for optimized character blitting
 */
static uint32_t	stamp[32];
static long	stamp_attr;
static int	stamp_mutex;	/* XXX see note in readme */

/*
 * offset = STAMP_SHIFT(fontbits, nibble #) & STAMP_MASK
 * destination uint32_t[0] = STAMP_READ(offset)
 * destination uint32_t[1] = STAMP_READ(offset + 4)
 */
#define	STAMP_SHIFT(fb, n)	((n) ? (fb) >> 1: (fb) << 3)
#define	STAMP_MASK		(0xf << 3)
#define	STAMP_READ(o)		(*(uint32_t *)((uint8_t *)stamp + (o)))
#endif

/*
 * Initialize rasops_info struct for this colordepth.
 */
void
rasops15_init(struct rasops_info *ri)
{

	if (ri->ri_rnum == 0) {
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 5;
		ri->ri_gnum += (ri->ri_depth == 16);

		ri->ri_rpos = 10 + (ri->ri_depth == 16);
		ri->ri_gpos = 5;
		ri->ri_bpos = 0;
	}

	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = rasops15_putchar_aa;
		return;
	}

	switch (ri->ri_font->fontwidth) {
#ifndef RASOPS_SMALL
	case 8:
		ri->ri_ops.putchar = rasops15_putchar8;
		break;
	case 12:
		ri->ri_ops.putchar = rasops15_putchar12;
		break;
	case 16:
		ri->ri_ops.putchar = rasops15_putchar16;
		break;
#endif	/* !RASOPS_SMALL */
	default:
		ri->ri_ops.putchar = rasops15_putchar;
		break;
	}
}

#define	RASOPS_DEPTH	15
#include "rasops_putchar.h"
#include "rasops_putchar_aa.h"

#ifndef RASOPS_SMALL
/*
 * Recompute the 4x1 blitting stamp.
 */
static void
rasops15_makestamp(struct rasops_info *ri, long attr)
{
	uint32_t fg, bg;
	int i;

	fg = ri->ri_devcmap[((uint32_t)attr >> 24) & 0xf] & 0xffff;
	bg = ri->ri_devcmap[((uint32_t)attr >> 16) & 0xf] & 0xffff;
	stamp_attr = attr;

	for (i = 0; i < 32; i += 2) {
#if BYTE_ORDER == LITTLE_ENDIAN
		stamp[i]      = (i & 16 ? fg : bg);
		stamp[i]     |= (i &  8 ? fg : bg) << 16;
		stamp[i + 1]  = (i &  4 ? fg : bg);
		stamp[i + 1] |= (i &  2 ? fg : bg) << 16;
#else
		stamp[i]      = (i &  8 ? fg : bg);
		stamp[i]     |= (i & 16 ? fg : bg) << 16;
		stamp[i + 1]  = (i &  2 ? fg : bg);
		stamp[i + 1] |= (i &  4 ? fg : bg) << 16;
#endif
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

#endif /* !RASOPS_SMALL */
