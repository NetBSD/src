/*	$NetBSD: pm2reg.h,v 1.3 2009/10/30 01:57:48 christos Exp $	*/

/*
 * Copyright (c) 2009 Michael Lorenz
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * register definitions for Permedia 2 graphics controllers
 */
 

#ifndef PM2_REG_H
#define PM2_REG_H

#define PM2_RESET	0x00000000	/* any write initiates a chip reset */
#define		PM2_RESET_BUSY	0x80000000	/* reset in progress */

#define PM2_INPUT_FIFO_SPACE	0x00000018
#define PM2_OUTPUT_FIFO_WORDS	0x00000020

#define PM2_APERTURE1_CONTROL	0x00000050
#define PM2_APERTURE2_CONTROL	0x00000058
#define		PM2_AP_BYTESWAP		0x00000001
#define		PM2_AP_HALFWORDSWAP	0x00000002
#define		PM2_AP_PACKED16_EN	0x00000008
#define		PM2_AP_PACKED16_READ_B	0x00000010 /* Buffer A otherwise */
#define		PM2_AP_PACKED16_WRITE_B	0x00000020 /* A otherwise */
#define		PM2_AP_PACKED16_WRT_DBL	0x00000040
#define		PM2_AP_PACKED16_R31	0x00000080 /* read buffer selected by
						    * visibility bit in memory 
						    */
#define		PM2_AP_SVGA		0x00000100
#define		PM2_AP_ROM		0x00000200

#define PM2_BYPASS_MASK		0x00001100
#define PM2_FB_WRITE_MASK	0x00001140

#define PM2_OUTPUT_FIFO		0x00002000

#define PM2_SCREEN_BASE		0x00003000 /* in 64bit units */
#define PM2_SCREEN_STRIDE	0x00003008 /* in 64bit units */

/* RAMDAC */
#define PM2_DAC_PAL_WRITE_IDX	0x00004000
#define PM2_DAC_DATA		0x00004008
#define PM2_DAC_MASK		0x00004010
#define PM2_DAC_PAL_READ	0x00004018
#define PM2_DAC_CURSOR_PAL	0x00004020
#define PM2_DAC_CURSOR_DATA	0x00004028
#define PM2_DAC_INDEX_DATA	0x00004050
#define PM2_DAC_CURSOR_RAM	0x00004058
#define PM2_DAC_CURSOR_X_LOW	0x00004060
#define PM2_DAC_CURSOR_X_HIGH	0x00004068
#define PM2_DAC_CURSOR_Y_LOW	0x00004070
#define PM2_DAC_CURSOR_Y_HIGH	0x00004078

/* drawing engine */
#define PM2_RE_BITMASK		0x00008068 /* for colour expansion */
#define PM2_RE_COLOUR		0x000087f0
#define PM2_RE_CONFIG		0x00008d90
#define		PM2RECFG_READ_SRC	0x00000001
#define		PM2RECFG_READ_DST	0x00000002
#define		PM2RECFG_PACKED		0x00000004
#define		PM2RECFG_WRITE_EN	0x00000008
#define		PM2RECFG_DDA_EN		0x00000010
#define		PM2RECFG_ROP_EN		0x00000020
#define		PM2RECFG_ROP_MASK	0x000003c0
#define		PM2RECFG_ROP_SHIFT	6

#define PM2_RE_CONST_COLOUR	0x000087e8
#define PM2_RE_BUFFER_OFFSET	0x00008a90 /* distance between src and dst */
#define PM2_RE_SOURCE_BASE	0x00008d80 /* write after windowbase */
#define PM2_RE_SOURCE_DELTA	0x00008d88 /* offset in coordinates */
#define PM2_RE_SOURCE_OFFSET	0x00008a88 /* same in pixels */
#define PM2_RE_WINDOW_BASE	0x00008ab0
#define PM2_RE_WRITE_MODE	0x00008ab8
#define		PM2WM_WRITE_EN		0x00000001
#define		PM2WM_TO_HOST		0x00000008

#define PM2_RE_MODE		0x000080a0
#define		PM2RM_MASK_MIRROR	0x00000001 /* mask is right-to-left */
#define		PM2RM_MASK_INVERT
#define		PM2RM_MASK_OPAQUE	0x00000040 /* BG in TEXEL0 */
#define		PM2RM_MASK_SWAP		0x00000180
#define		PM2RM_MASK_PAD		0x00000200 /* new line new mask */
#define		PM2RM_MASK_OFFSET	0x00007c00
#define		PM2RM_HOST_SWAP		0x00018000
#define		PM2RM_LIMITS_EN		0x00040000
#define		PM2RM_MASK_REL_X	0x00080000

#define PM2_RE_RECT_START	0x000080d0
#define PM2_RE_RECT_SIZE	0x000080d8
#define PM2_RE_RENDER		0x00008038 /* write starts command */
#define		PM2RE_STIPPLE		0x00000001
#define		PM2RE_FASTFILL		0x00000008
#define		PM2RE_LINE		0x00000000
#define		PM2RE_TRAPEZOID		0x00000040
#define		PM2RE_POINT		0x00000080
#define		PM2RE_RECTANGLE		0x000000c0
#define		PM2RE_SYNC_ON_MASK	0x00000800 /* wait for write to bitmask
						      register */
#define		PM2RE_SYNC_ON_HOST	0x00001000 /* wait for host data */
#define		PM2RE_TEXTURE_EN	0x00002000
#define		PM2RE_INC_X		0x00200000 /* drawing direction */
#define		PM2RE_INC_Y		0x00400000
#define	PM2_RE_TEXEL0		0x00008600 /* background colour */
#define PM2_RE_STATUS		0x00000068
#define		PM2ST_BUSY	0x80000000
#define PM2_RE_SYNC		0x00008c40
#define PM2_RE_FILTER_MODE	0x00008c00
#define		PM2FLT_PASS_SYNC	0x00000400
#define PM2_RE_DDA_MODE		0x000087e0
#define		PM2DDA_ENABLE		0x00000001
#define		PM2DDA_GOURAUD		0x00000002 /* flat otherwise */
#define PM2_RE_BLOCK_COLOUR	0x00008ac8
#endif /* PM2_REG_H */
