/*	$NetBSD: grf_etreg.h,v 1.1.116.1 2007/03/12 05:47:20 rmind Exp $	*/

/*
 * Copyright (c) 1996 Tobias Abt
 * Copyright (c) 1995 Ezra Story
 * Copyright (c) 1995 Kari Mettinen
 * Copyright (c) 1994 Markus Wild
 * Copyright (c) 1994 Lutz Vieweg
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Lutz Vieweg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _GRF_ETREG_H
#define _GRF_ETREG_H

/*
 * Written & Copyright by Kari Mettinen, Ezra Story.
 *
 * This is derived from Cirrus driver source
 */

/*
 * read/write VGA registers
 */
#define vgar(ba, reg)		(*(((volatile u_char *)ba)+reg))
#define vgaw(ba, reg, val)	*(((volatile u_char *)ba)+reg) = ((u_char)val)

/*
 * defines for the used register addresses (mw)
 *
 * NOTE: there are some registers that have different addresses when
 *       in mono or color mode. We only support color mode, and thus
 *       some addresses won't work in mono-mode!
 *
 * General and VGA-registers taken from retina driver. Fixed a few
 * bugs in it. (SR and GR read address is Port + 1, NOT Port)
 *
 */

/* General Registers: */
#define GREG_STATUS0_R		0x03C2
#define GREG_STATUS1_R		0x03DA
#define GREG_MISC_OUTPUT_R	0x03CC
#define GREG_MISC_OUTPUT_W	0x03C2
#define GREG_FEATURE_CONTROL_R	0x03CA
#define GREG_FEATURE_CONTROL_W	0x03DA
#define GREG_POS		0x0102
#define	GREG_HERCULESCOMPAT	0x03BF
#define	GREG_VIDEOSYSENABLE	0x03C3
#define	GREG_DISPMODECONTROL	0x03D8
#define	GREG_COLORSELECT	0x03D9
#define	GREG_ATNTMODECONTROL	0x03DE
#define	GREG_SEGMENTSELECT	0x03CD

/* Attribute Controller: */
#define ACT_ADDRESS		0x03C0
#define ACT_ADDRESS_R		0x03C1
#define ACT_ADDRESS_W		0x03C0
#define ACT_ADDRESS_RESET	0x03DA
#define ACT_ID_PALETTE0		0x00
#define ACT_ID_PALETTE1		0x01
#define ACT_ID_PALETTE2		0x02
#define ACT_ID_PALETTE3		0x03
#define ACT_ID_PALETTE4		0x04
#define ACT_ID_PALETTE5		0x05
#define ACT_ID_PALETTE6		0x06
#define ACT_ID_PALETTE7		0x07
#define ACT_ID_PALETTE8		0x08
#define ACT_ID_PALETTE9		0x09
#define ACT_ID_PALETTE10	0x0A
#define ACT_ID_PALETTE11	0x0B
#define ACT_ID_PALETTE12	0x0C
#define ACT_ID_PALETTE13	0x0D
#define ACT_ID_PALETTE14	0x0E
#define ACT_ID_PALETTE15	0x0F
#define ACT_ID_ATTR_MODE_CNTL	0x10
#define ACT_ID_OVERSCAN_COLOR	0x11
#define ACT_ID_COLOR_PLANE_ENA	0x12
#define ACT_ID_HOR_PEL_PANNING	0x13
#define ACT_ID_COLOR_SELECT	0x14
#define	ACT_ID_MISCELLANEOUS	0x16

/* Graphics Controller: */
#define GCT_ADDRESS		0x03CE
#define GCT_ADDRESS_R		0x03CF
#define GCT_ADDRESS_W		0x03CF
#define GCT_ID_SET_RESET	0x00
#define GCT_ID_ENABLE_SET_RESET	0x01
#define GCT_ID_COLOR_COMPARE	0x02
#define GCT_ID_DATA_ROTATE	0x03
#define GCT_ID_READ_MAP_SELECT	0x04
#define GCT_ID_GRAPHICS_MODE	0x05
#define GCT_ID_MISC		0x06
#define GCT_ID_COLOR_XCARE	0x07
#define GCT_ID_BITMASK		0x08

/* Sequencer: */
#define SEQ_ADDRESS		0x03C4
#define SEQ_ADDRESS_R		0x03C5
#define SEQ_ADDRESS_W		0x03C5
#define SEQ_ID_RESET		0x00
#define SEQ_ID_CLOCKING_MODE	0x01
#define SEQ_ID_MAP_MASK		0x02
#define SEQ_ID_CHAR_MAP_SELECT	0x03
#define SEQ_ID_MEMORY_MODE	0x04
#define	SEQ_ID_STATE_CONTROL	0x06
#define	SEQ_ID_AUXILIARY_MODE	0x07

/* CRT Controller: */
#define CRT_ADDRESS		0x03D4
#define CRT_ADDRESS_R		0x03D5
#define CRT_ADDRESS_W		0x03D5
#define CRT_ID_HOR_TOTAL	0x00
#define CRT_ID_HOR_DISP_ENA_END	0x01
#define CRT_ID_START_HOR_BLANK	0x02
#define CRT_ID_END_HOR_BLANK	0x03
#define CRT_ID_START_HOR_RETR	0x04
#define CRT_ID_END_HOR_RETR	0x05
#define CRT_ID_VER_TOTAL	0x06
#define CRT_ID_OVERFLOW		0x07
#define CRT_ID_PRESET_ROW_SCAN	0x08
#define	CRT_ID_MAX_ROW_ADDRESS	0x09
#define CRT_ID_CURSOR_START	0x0A
#define CRT_ID_CURSOR_END	0x0B
#define CRT_ID_START_ADDR_HIGH	0x0C
#define CRT_ID_START_ADDR_LOW	0x0D
#define CRT_ID_CURSOR_LOC_HIGH	0x0E
#define CRT_ID_CURSOR_LOC_LOW	0x0F
#define CRT_ID_START_VER_RETR	0x10
#define CRT_ID_END_VER_RETR	0x11
#define CRT_ID_VER_DISP_ENA_END	0x12
#define CRT_ID_OFFSET		0x13
#define CRT_ID_UNDERLINE_LOC	0x14
#define CRT_ID_START_VER_BLANK	0x15
#define CRT_ID_END_VER_BLANK	0x16
#define CRT_ID_MODE_CONTROL	0x17
#define CRT_ID_LINE_COMPARE	0x18

#define	CRT_ID_SEGMENT_COMP	0x30
#define	CRT_ID_GENERAL_PURPOSE	0x31
#define	CRT_ID_RASCAS_CONFIG	0x32
#define	CTR_ID_EXT_START	0x33
#define	CRT_ID_6845_COMPAT	0x34
#define	CRT_ID_OVERFLOW_HIGH	0x35
#define	CRT_ID_VIDEO_CONFIG1	0x36
#define	CRT_ID_VIDEO_CONFIG2	0x37
#define	CRT_ID_HOR_OVERFLOW	0x3f

/* Video DAC */
#define VDAC_ADDRESS		0x03c8
#define VDAC_ADDRESS_W		0x03c8
#define VDAC_ADDRESS_R		0x03c7
#define VDAC_STATE		0x03c7
#define VDAC_DATA		0x03c9
#define VDAC_MASK		0x03c6
#define HDR			0x03c6	/* Hidden DAC regs, 4 reads to access */

#define VDAC_COMMAND		0x03c6
#define VDAC_XINDEX		0x03c7
#define VDAC_XDATA		0x03c8

#define WGfx(ba, idx, val)			\
	do {					\
		vgaw(ba, GCT_ADDRESS, idx);	\
		vgaw(ba, GCT_ADDRESS_W , val);	\
	} while (0)

#define WSeq(ba, idx, val)			\
	do {					\
		vgaw(ba, SEQ_ADDRESS, idx);	\
		vgaw(ba, SEQ_ADDRESS_W , val);	\
	} while (0)

#define WCrt(ba, idx, val)			\
	do {					\
		vgaw(ba, CRT_ADDRESS, idx);	\
		vgaw(ba, CRT_ADDRESS_W , val);	\
	} while (0)

#define WIma(ba, idx, val)			\
	do {					\
		vgaw(ba, IMA_ADDRESS, idx);	\
		vgaw(ba, IMA_ADDRESS_W , val);	\
	} while (0)

#define WAttr(ba, idx, val)			\
	do {					\
		if(vgar(ba, GREG_STATUS1_R))	\
			;			\
		vgaw(ba, ACT_ADDRESS_W, idx);	\
		vgaw(ba, ACT_ADDRESS_W, val);	\
	} while (0)

static inline u_char RAttr(volatile void *ba, short idx) {
	if(vgar(ba, GREG_STATUS1_R))
		;
	vgaw(ba, ACT_ADDRESS_W, idx);
	return vgar(ba, ACT_ADDRESS_R);
}

static inline u_char RSeq(volatile void *ba, short idx) {
	vgaw(ba, SEQ_ADDRESS, idx);
	return vgar(ba, SEQ_ADDRESS_R);
}

static inline u_char RCrt(volatile void *ba, short idx) {
	vgaw(ba, CRT_ADDRESS, idx);
	return vgar(ba, CRT_ADDRESS_R);
}

static inline u_char RGfx(volatile void *ba, short idx) {
	vgaw(ba, GCT_ADDRESS, idx);
	return vgar(ba, GCT_ADDRESS_R);
}

#endif /* _GRF_ETREG_H */
