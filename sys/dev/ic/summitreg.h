/*	$NetBSD: summitreg.h,v 1.7 2024/12/05 12:37:16 macallan Exp $	*/

/*
 * Copyright (c) 2024 Michael Lorenz
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* HP Visualize FX 4 and related hardware, aka Summit */

/*
 * register values, found by disassembling the ROM
 * some found by Sven Schnelle
 * ( see https://patchwork.kernel.org/project/linux-parisc/patch/20211031204952.25678-2-svens@stackframe.org/ )
 * some by me
 */

#ifndef SUMMITREG_H
#define SUMMITREG_H

#define VISFX_STATUS		0x641400	// zero when idle
#define VISFX_FIFO		0x641440
#define VISFX_VRAM_WRITE_MODE	0xa00808
#define VISFX_VRAM_READ_MODE	0xa0080c	// this is a guess
#define VISFX_PIXEL_MASK	0xa0082c
#define VISFX_FG_COLOUR		0xa0083c
#define VISFX_BG_COLOUR		0xa00844
#define VISFX_PLANE_MASK	0xa0084c
/* this controls what we see in the FB aperture */
#define VISFX_APERTURE_ACCESS	0xa00858
	#define VISFX_DEPTH_8	0xb0
	#define VISFX_DEPTH_32	0xd0

#define VISFX_VRAM_WRITE_DATA_INCRX	0xa60000
#define VISFX_VRAM_WRITE_DATA_INCRY	0xa68000
#define VISFX_VRAM_WRITE_DEST		0xac1000

#define VISFX_CLIP_TL		0xac1050	/* clipping rect, top/left */
#define VISFX_CLIP_WH		0xac1054	/* clipping rect, w/h */

#define VISFX_WRITE_MODE_PLAIN	0x02000000
#define VISFX_WRITE_MODE_EXPAND	0x050004c0
#define VISFX_WRITE_MODE_FILL	0x050008c0
#define VISFX_WRITE_MODE_TRANSPARENT	0x00000800	/* bg is tansparent */
#define VISFX_WRITE_MODE_MASK		0x00000400	/* apply pixel mask */
/* 0x00000200 - some pattern */
/* looks like 0x000000c0 enables fb/bg colours to be applied */

#define VISFX_READ_MODE_COPY	0x02000400

/*
 * for STI colour change mode:
 * set VISFX_FG_COLOUR, VISFX_BG_COLOUR
 * set VISFX_VRAM_READ_MODE 0x05000400
 * set VISFX_VRAM_WRITE_MODE 0x050000c0
 */

/* fill */
#define VISFX_START		0xb3c000
#define VISFX_SIZE		0xb3c808	/* start, FX4 uses 0xb3c908 */

/* copy */
#define VISFX_COPY_SRC		0xb3c010
#define VISFX_COPY_WH		0xb3c008
#define VISFX_COPY_DST		0xb3cc00
/*
 * looks like ORing 0x800 to the register address starts a command
 * - 0x800 - fill
 * - 0xc00 - copy
 * 0x100 and 0x200 seem to have functions as well, not sure what though
 * for example, the FX4 ROM uses 0xb3c908 to start a rectangle fill, but
 * it also works with 0xb3c808 and 0xb3ca08
 * same with copy, 0xc00 seems to be what matters, setting 0x100 or 0x200
 * doesn't seem to make a difference
 * 0x400 or 0x100 by themselves don't start a command either
 */

#define VISFX_COLOR_MASK	0x800018
#define VISFX_COLOR_INDEX	0x800020
#define VISFX_COLOR_VALUE	0x800024

#define VISFX_CURSOR_POS	0x400000
#define VISFX_CURSOR_INDEX	0x400004
#define VISFX_CURSOR_DATA	0x400008
#define VISFX_CURSOR_COLOR	0x400010
#define VISFX_CURSOR_ENABLE	0x80000000

#endif	/* SUMMITREG_H */
