/* $NetBSD: vidcreg.h,v 1.2 2002/03/24 23:37:45 bjh21 Exp $ */

/*-
 * Copyright (c) 1998, 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * vidcreg.h - Acorn/ARM VIDC (Arabella) registers
 */

#ifndef _ARM26_VIDCREG_H
#define _ARM26_VIDCREG_H

/*
 * The VIDC is accessed by writing words to addresses starting at
 * MEMC_VIDC_BASE definied in memcreg.h.
 *
 * As with the MEMC, the value is the logical OR of the register
 * specifier and the new value.
 */

/* Palette entries */

#define VIDC_PALETTE		0x00000000
#define VIDC_PALETTE_LCOL(n)	(n << 26)
#define VIDC_PALETTE_BCOL	0x40000000
#define VIDC_PALETTE_CCOL(n)	(0x40000000 + (n << 26))

#define VIDC_PALETTE_ENTRY(r, g, b, s)	((s != 0) << 12 | b << 8 | g << 4 | r)

/* Stereo image registers */
/* They're in the order 70123456 */
#define VIDC_SIR(n)		(0x60000000 + ((n + 1) % 8 << 26))
#define VIDC_STEREO_L100	1
#define VIDC_STEREO_L83		2
#define VIDC_STEREO_L67		3
#define VIDC_STEREO_C		4
#define VIDC_STEREO_R67		5
#define VIDC_STEREO_R83		6
#define VIDC_STEREO_R100	7

/* Video timing register */
#define VIDC_HCR		0x80000000
#define VIDC_HSWR		0x84000000
#define VIDC_HBSR		0x88000000
#define VIDC_HDSR		0x8c000000
#define VIDC_HDER		0x90000000
#define VIDC_HBER		0x94000000
#define VIDC_HCSR		0x98000000
#define VIDC_HIR		0x9c000000

#define VIDC_VCR		0xa0000000
#define VIDC_VSWR		0xa4000000
#define VIDC_VBSR		0xa8000000
#define VIDC_VDSR		0xac000000
#define VIDC_VDER		0xb0000000
#define VIDC_VBER		0xb4000000
#define VIDC_VCSR		0xb8000000
#define VIDC_VCER		0xbc000000

/*
 * Horizontal timings have units of two pixels (except HCSR).
 * Vertical timings have units of a raster.  Most have to have one or
 * two subtracted from them.
 */
#define VIDC_VIDTIMING(x)	(x << 14)
#define VIDC_HCS(x)		(x << 13)
#define VIDC_HCS_HIRES(x)	(x << 11)

/* Sound frequency register */
#define VIDC_SFR		0xc0000000
/* Units are us */
#define VIDC_SF(x)		(x & 0x100)

/* Control register */
#define VIDC_CONTROL		0xe0000000

#define VIDC_CTL_DOTCLOCK_MASK	0x00000003
#define VIDC_CTL_DOTCLOCK_8MHZ	0x00000000
#define VIDC_CTL_DOTCLOCK_12MHZ	0x00000001
#define VIDC_CTL_DOTCLOCK_16MHZ	0x00000002
#define VIDC_CTL_DOTCLOCK_24MHZ	0x00000003

#define VIDC_CTL_BPP_MASK	0x0000000c
#define VIDC_CTL_BPP_ONE       	0x00000000
#define VIDC_CTL_BPP_TWO	0x00000004
#define VIDC_CTL_BPP_FOUR	0x00000008
#define VIDC_CTL_BPP_EIGHT	0x0000000c

#define VIDC_CTL_DMARQ_MASK	0x00000030
#define VIDC_CTL_DMARQ_04	0x00000000
#define VIDC_CTL_DMARQ_15	0x00000010
#define VIDC_CTL_DMARQ_26	0x00000020
#define VIDC_CTL_DMARQ_37	0x00000030

#define VIDC_CTL_INTERLACE	0x00000040

#define VIDC_CTL_CSYNC		0x00000080

#define VIDC_CTL_TEST_MASK	0x0000c100
#define VIDC_CTL_TEST_OFF	0x00000000
#define VIDC_CTL_TEST_MODE0	0x00004000
#define VIDC_CTL_TEST_MODE1	0x00008000
#define VIDC_CTL_TEST_MODE2	0x0000c000
#define VIDC_CTL_TEST_MODE3	0x00000100

#define VIDC_WRITE(value)	*(volatile u_int32_t *)MEMC_VIDC_BASE = value

/*
 * VIDC audio format is mu-law, but with the bits in a strange order.
 *
 * VIDC1 has:
 * D[7]   sign
 * D[6:4] chord select
 * D[3:0] point on chord
 * so 0x00 -> +0, 0x7f -> +inf, 0x80 -> -0, 0xff -> -inf
 *
 * VIDC2 has:
 * D[7:5] chord select
 * D[4:1] point on chord
 * D[0]   sign
 * so 0x00 -> +0, 0xfe -> +inf, 0x01 -> -0, 0xff -> -inf
 *
 * Normal mu-law appears to have:
 * 0x00 -> -inf, 0x7f -> -0, 0x80 -> +inf, 0xff -> +0
 * Thus VIDC1 is NOT(mu-law), while VIDC2 is NOT(mu-law)<<1 | NOT(mu-law)>>7.
 *
 * I think the A500 uses VIDC1 and the Archimedes uses VIDC2.
 */

#endif
