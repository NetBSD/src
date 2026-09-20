/*	$NetBSD: nwb225reg.h,v 1.2 2026/09/20 11:52:56 tsutsui Exp $	*/

/*-
 * Copyright (c) 2026 Izumi Tsutsui.  All rights reserved.
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

#ifndef _NEWS68K_NWB225REG_H_
#define _NEWS68K_NWB225REG_H_

/* NWB-225 MMIO addresses */
#define NWB225_KROM_BASE		0xf0600000UL
#define NWB225_KROM_SIZE		0x00100000UL
#define NWB225_RCONT_BASE		0xf0700000UL
#define NWB225_RCONT_SIZE		0x00100000UL

/*
 * ROP controller register/aperture offsets from NWB225_RCONT_BASE
 */

/* Status: ROP status register */
#define NWB225_STATUS			0x00080000UL
#define  NWB225_STATUS_REQ		0x0001
#define  NWB225_STATUS_EXEC		0x0002
#define  NWB225_STATUS_BUSY		(NWB225_STATUS_EXEC | NWB225_STATUS_REQ)
#define  NWB225_STATUS_DISPVER		0x00e0
#define  NWB225_STATUS_DISPVER_SHIFT	5
#define  NWB225_STATUS_4PLANE		0x0080

/* Command-address flags: A13 Set Shift / A14 Execute */
#define NWB225_ROP_ADDR_FLAG_SET_SHIFT	0x00002000UL
#define NWB225_ROP_ADDR_FLAG_EXECUTE	0x00004000UL

/* Command-addresses: COPY and FILL */
#define NWB225_ROP_COPY_XR_YR		0x00060300UL
#define NWB225_ROP_COPY_XF_YR		0x00060310UL
#define NWB225_ROP_COPY_XR_YF		0x00060320UL
#define NWB225_ROP_COPY_XF_YF		0x00060330UL
#define NWB225_ROP_CONST_FILL		0x000603b0UL

/* Parameter-addresses: rectangles of COPY, FILL, and HWRITE */
/* y must be in the range 0..NWB225_FB_HEIGHT - 1 */
#define NWB225_DST_ADDR(y)		(0x00008000UL | 2UL * (y))
#define NWB225_SRC_ADDR(y)		(0x00020000UL | 2UL * (y))
/* h must be in the range 1..NWB225_FB_HEIGHT */
#define NWB225_HWRITE_LIMIT(h)		(0x00040000UL | 2UL * ((h) - 1))
#define NWB225_ROP_LIMIT(h)		(0x00050000UL | 2UL * ((h) - 1))

/* Parameter-addresses: ROP func of COPY, FILL, and HWRITE */
/* p must be in the range 0..7 and less than dc_nplanes */
#define NWB225_ROP_FUNC(p)		(0x00060730UL | 2UL * (p))

/* Parameter-addresses: for host data of HWRITE */
#define NWB225_HWRITE_DATA		0x00070230UL
#define NWB225_HWRITE_BITS_PER_WORD	16

#define NWB225_PLANE_CTRL		0x000a0000UL
#define NWB225_PLANE_CTRL_SHIFT		8

#define NWB225_CTL_SELECT		0x000c0000UL
#define NWB225_PAL_INDEX		0x000c0000UL
#define NWB225_PAL_DATA			0x000c0002UL
#define NWB225_CTL_DATA			0x000c0004UL
#define NWB225_INIT_BASE		0x000e0000UL

/* Fixed hardware geometry used by the NWB-225 NEWS-OS/PROM setup */
#define NWB225_FB_WIDTH			2048
#define NWB225_FB_HEIGHT		1024
#define NWB225_VIS_WIDTH		1280
#define NWB225_VIS_HEIGHT		1024

/* NEWS-OS KROM glyph and text-cell geometry */
#define NWB225_FONT_WIDTH		12
#define NWB225_FONT_HEIGHT		24
#define NWB225_CELL_WIDTH		16
#define NWB225_CELL_HEIGHT		30
#define NWB225_GLYPH_YOFFSET		4

#define NWB225_KROM_ASCII_FIRST		0x20
#define NWB225_KROM_ASCII_LAST		0x7e
#define NWB225_KROM_ROWBYTES		4
#define NWB225_KROM_ASCII_MASK		0xfff0
#define NWB225_TEXT_COLS		80
#define NWB225_TEXT_ROWS		33
#define NWB225_TEXT_XORIGIN		0
#define NWB225_TEXT_YORIGIN		8

#endif /* _NEWS68K_NWB225REG_H_ */
