/*	$NetBSD: summitreg.h,v 1.14 2025/01/26 05:20:57 macallan Exp $	*/

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

#define VISFX_CONTROL		0x641000
	#define CONTROL_WFC	0x00000200	// FIFO when 0, direct when 1
#define VISFX_FC		0x641040	// Fault Control
#define VISFX_STATUS		0x641400	// zero when idle
/*
 * about the FIFO register:
 * - on FX4, there are 0x800 FIFO slots, quite a lot
 * - based on observation, every register write seems to occupy *two* slots
 * - we need to write 0 to VISFX_CONTROL to enable FIFO pacing
 * - the FIFO is quite difficult to overrun but things like x11perf copywinwin
 *   will do it if we're not careful
 */
#define VISFX_FIFO		0x641440
#define VISFX_FOE		0x920404	// Fragment Operation Enable
	#define FOE_TEXTURE	0x00000001
	#define FOE_SPECULAR	0x00000002
	#define FOE_DEPTHCUE	0x00000004
	#define FOE_ALPHATEST	0x00000008
	#define FOE_STENCIL	0x00000010
	#define FOE_Z_TEST	0x00000020
	#define FOE_BLEND_ROP	0x00000040	// IBO is used
	#define FOE_DITHER	0x00000080
#define VISFX_IBO		0x921110	// ROP in lowest nibble
#define VISFX_IAA0		0x921200	// XLUT, 16 entries
#define VISFX_IAA(n)		(0x921200 + ((n) << 2))
#define VISFX_OTR		0x921148	// overlay transparency

#define VISFX_VRAM_WRITE_MODE	0xa00808
#define VISFX_VRAM_READ_MODE	0xa0080c
#define VISFX_PIXEL_MASK	0xa0082c
#define VISFX_FG_COLOUR		0xa0083c
#define VISFX_BG_COLOUR		0xa00844
#define VISFX_PLANE_MASK	0xa0084c
/* this controls what we see in the FB aperture */
#define VISFX_APERTURE_ACCESS	0xa00858
	#define VISFX_DEPTH_8	0x30
	#define VISFX_DEPTH_32	0x50
#define VISFX_RPH		0xa0085c	// read prefetch hint
	#define VISFX_RPH_RTL	0x80000000	// right-to-left
	#define VISFX_RPH_LTR	0x00000000	// left-to-right

#define VISFX_READ_DATA		0xa41480

#define VISFX_VRAM_WRITE_DATA_INCRX	0xa60000
#define VISFX_VRAM_WRITE_DATA_INCRY	0xa68000
#define VISFX_VRAM_WRITE_DEST		0xac1000
#define VISFX_TCR			0xac1024	/* throttle control */
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

#define OTC01	0x00000000	/* one pixel per 32bit write */
#define OTC04	0x02000000	/* 4 pixels per 32bit write */
#define OTC32	0x05000000	/* 32 pixels per 32bit write */
#define BIN8I	0x00000000	/* 8bit indexed */
#define BIN12I	0x00010000	/* 12bit indexed */
#define BIN332F	0x00040000	/* R3G3B2 */
#define BIN8F	0x00070000	/* ARGB8 */
#define BINapln	0x00110000	/* attribute plane */
#define BINhost	0x00300000	/* DMA to host */
#define BUFovl	0x00000000	/* 8bit overlay */
#define BUFBL	0x00008000	/* back/left */
#define BUFFL	0x00004000	/* front/left */
#define BUFBR	0x00002000	/* back/right */
#define BUFFR	0x00001000	/* front/right */

/* attribute table, this only selects depth and CFS */
#define IAA_8I		0x00000000	/* 8bit CI */
#define IAA_8F		0x00000070	/* RGB8 */
#define IAA_CFS0	0x00000000	/* CFS select */
#define IAA_CFS1	0x00000100	/* CFS 1 etc. */

#define OTR_T	0x00010000	/* when set 0 is transparent, otherwise 0xff */
#define OTR_A	0x00000100	/* always transparent */
#define OTR_L1	0x00000002	/* transparency controlled by CFS17 */
#define OTR_L0	0x00000001	/* transparency controlled by CFS16 */

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

/*
 * use unbuffered space for cursor registers
 * The _POS, _INDEX and _DATA registers work exactly like on HCRX
 */

#define VISFX_CURSOR_POS	0x400000
#define VISFX_CURSOR_ENABLE	0x80000000
#define VISFX_CURSOR_INDEX	0x400004
#define VISFX_CURSOR_DATA	0x400008
#define VISFX_CURSOR_FG		0x40000c
#define VISFX_CURSOR_BG		0x400010
#define VISFX_COLOR_MASK	0x800018
#define VISFX_COLOR_INDEX	0x800020
#define VISFX_COLOR_VALUE	0x800024
#define VISFX_FATTR		0x80003c	/* force attribute */
#define VISFX_MPC		0x80004c
	#define MPC_VIDEO_ON	0x0c
	#define MPC_VSYNC_OFF	0x02
	#define MPC_HSYNC_OFF	0x01
#define VISFX_CFS0		0x800100	/* colour function select */
#define VISFX_CFS(n)		(VISFX_CFS0 + ((n) << 2))
/* 0 ... 6 for image planes, 7 or bypass, 16 and 17 for overlay */
#define CFS_CR		0x80	// enable color recovery
#define CFS_332		0x00	// R3G3B2
#define CFS_8I	 	0x40	// 8bit indexed
#define CFS_8F		0x70	// ARGB8
#define CFS_LUT0	0x00	// use LUT 0
#define CFS_LUT1	0x01	// LUT 1 etc.
#define CFS_BYPASS	0x07	// bypass LUT

#endif	/* SUMMITREG_H */
