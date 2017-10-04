/*	$NetBSD: mntvareg.h,v 1.2 2017/10/04 09:44:09 rkujawa Exp $  */

/*
 * Copyright (c) 2012, 2016 The NetBSD Foundation, Inc. 
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lukas F. Hartmann.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef MNTVA2000REG_H
#define MNTVA2000REG_H

/* address space */
#define MNTVA_OFF_REG		0x000000	/* memory mapped registers */
#define MNTVA_REG_SIZE	0x001000

#define MNTVA_OFF_FB		0x010000	/* frame buffer */
#define MNTVA_FB_SIZE		0x5f0000

/* registers */
#define MNTVA_SCALEMODE		0x04
#define MNTVA_SCALEMODE1X		0
#define MNTVA_SCALEMODE2X		1
#define MNTVA_SCALEMODE3X		2	
#define MNTVA_SCALEMODE4X		3	

#define MNTVA_LINEW     0x02
#define MNTVA_SCREENW		0x06
#define MNTVA_SCREENH		0x08
#define MNTVA_MARGIN_X  0x0c
#define MNTVA_SAFE_X    0x14

#define MNTVA_BLITTERBASEHI	0x1C	/* [23:16] */
#define MNTVA_BLITTERBASELO	0x1E	/* [15:0] */

#define MNTVA_BLITTERX1		0x20
#define MNTVA_BLITTERY1		0x22
#define MNTVA_BLITTERX2		0x24
#define MNTVA_BLITTERY2		0x26
#define MNTVA_BLITTERRGB	0x28	/* filling for 16bit and 8bit modes */

#define MNTVA_BLITTER_ENABLE	0x2A
#define MNTVA_BLITTER_FILL		__BIT(0) /* fill [x1,y1]-[x2,y2] */
#define MNTVA_BLITTER_COPY		__BIT(1) /* copy [x3,y3]-[x4,y4] to
						         [x1,y1]-[x2,y2] */
#define MNTVA_BLITTERX3		0x2C
#define MNTVA_BLITTERY3		0x2E
#define MNTVA_BLITTERX4		0x30
#define MNTVA_BLITTERY4		0x32
#define MNTVA_BLITTERRGB32HI	0x34	/* filling for 24bit and 32bit modes */
#define MNTVA_BLITTERRGB32LO	0x36	/* filling for 24bit and 32bit modes */

#define MNTVA_COLORMODE 	0x48
#define MNTVA_COLORMODE8		0
#define MNTVA_COLORMODE16		__BIT(0)	
#define MNTVA_COLORMODE32		__BIT(1)

#define MNTVA_PANPTRHI 0x38 /* [23:16] */
#define MNTVA_PANPTRLO 0x3A /* [15:0] */

#define MNTVA_BLITTERX1		0x20
#define MNTVA_BLITTERY1		0x22
#define MNTVA_BLITTERX2		0x24
#define MNTVA_BLITTERY2		0x26
#define MNTVA_BLITTERRGB	0x28	/* filling for 16bit and 8bit modes */

#define MNTVA_BLITTER_ENABLE	0x2A
#define MNTVA_BLITTER_FILL		__BIT(0) /* fill [x1,y1]-[x2,y2] */
#define MNTVA_BLITTER_COPY		__BIT(1) /* copy [x3,y3]-[x4,y4] to
						         [x1,y1]-[x2,y2] */
#define MNTVA_BLITTERX3		0x2C
#define MNTVA_BLITTERY3		0x2E
#define MNTVA_BLITTERX4		0x30
#define MNTVA_BLITTERY4		0x32
#define MNTVA_BLITTERRGB32HI	0x34	/* filling for 24bit and 32bit modes */
#define MNTVA_BLITTERRGB32LO	0x36	/* filling for 24bit and 32bit modes */

#define MNTVA_COLORMODE 	0x48
#define MNTVA_COLORMODE8		0
#define MNTVA_COLORMODE16		__BIT(0)	
#define MNTVA_COLORMODE32		__BIT(1)

#define MNTVA_BLITTER_ROW_PITCH 0x42
#define MNTVA_BLITTER_ROW_PITCH_SHIFT 0x44
#define MNTVA_BLITTER_COLORMODE 0x46

#define MNTVA_PANPTRHI 0x38 /* [23:16] */
#define MNTVA_PANPTRLO 0x3A /* [15:0] */

#define MNTVA_CAPTURE_MODE 0x4E

#define MNTVA_ROW_PITCH       0x58
#define MNTVA_ROW_PITCH_SHIFT 0x5c

#define MNTVA_H_SYNC_START  0x70
#define MNTVA_H_SYNC_END    0x72
#define MNTVA_H_MAX         0x74
#define MNTVA_V_SYNC_START  0x76
#define MNTVA_V_SYNC_END    0x78
#define MNTVA_V_MAX         0x7a

#define MNTVA_PIXEL_CLK_SEL 0x7c
#define MNTVA_CLK_75MHZ 0
#define MNTVA_CLK_40MHZ 1

#endif /* MNTVA2000REG_H */
