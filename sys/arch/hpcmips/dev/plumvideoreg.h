/*	$NetBSD: plumvideoreg.h,v 1.1 1999/11/21 06:50:27 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* (CS3) */
#define	PLUM_VIDEO_REGBASE		0x1000
#define	PLUM_VIDEO_REGSIZE		0x200

/* (MCS0) */
/* VRAM 4MByte */
#define PLUM_VIDEO_VRAM_IOBASE		0x00000000
#define PLUM_VIDEO_VRAM_IOSIZE		0x00400000
/* Color palette LCD 4KByte */
#define	PLUM_VIDEO_CLUT_LCD_IOBASE	0x00400000
#define	PLUM_VIDEO_CLUT_LCD_IOSIZE	0x00001000
/* Color palette CRT 4KByte */
#define	PLUM_VIDEO_CLUT_CRT_IOBASE	0x00401000
#define	PLUM_VIDEO_CLUT_CRT_IOSIZE	0x00001000
/* BitBlt 4KByte */
#define PLUM_VIDEO_BITBLT_IOBASE	0x00402000
#define PLUM_VIDEO_BITBLT_IOSIZE	0x00401000

/*
 *	Common Control Register
 */
/* Interrupt Status enable and IRQ line enable */
#define	PLUM_VIDEO_POSENIEN_REG		0x000
/* Interrupt Status */
#define	PLUM_VIDEO_POIST_REG		0x004
/* Buffer Control */
#define	PLUM_VIDEO_POBFC_REG		0x008
/* VRAM Control */
#define	PLUM_VIDEO_PORAM_REG		0x00c
/* VRAM Refresh Control */
#define	PLUM_VIDEO_POREF_REG		0x010
/* LCD Clock Source select and control */
#define	PLUM_VIDEO_POCKL_REG		0x014
/* CRT Clock Source select and control */
#define	PLUM_VIDEO_POCKC_REG		0x018
/* PLL Clock Source select and control */
#define	PLUM_VIDEO_POPLL_REG		0x01c

/*
 *	LCD Panel Control Register
 */
/* LCD Control */
#define	PLUM_VIDEO_PLCNT_REG		0x040
/* STN Control */
#define	PLUM_VIDEO_PLSTN_REG		0x044
/* LCD Level control */
#define	PLUM_VIDEO_PLLEV_REG		0x048
/* LCD Luminance control */
#define	PLUM_VIDEO_PLLUM_REG		0x04c
/* DSTN Dither Pattern base address */
#define	PLUM_VIDEO_PLDPA_REG		0x050
/* DSTN VRAM Offscreen buffer address */
#define	PLUM_VIDEO_PLOSA_REG		0x054

/*
 *	CRT Control Register
 */
/* DAC Control */
#define	PLUM_VIDEO_PCDAC_REG		0x060
/* CRT Border Color */
#define	PLUM_VIDEO_PCBOC_REG		0x064
/* Palette snoop */
#define	PLUM_VIDEO_PCSNP_REG		0x068

/*
 *	LCD Timing Register
 */
/* Horizontanl Total */
#define	PLUM_VIDEO_PLHT_REG		0x080
/* Horizontal Display Start */
#define	PLUM_VIDEO_PLHDS_REG		0x084
/* H-Sync Start/End */
#define	PLUM_VIDEO_PLHSEHSS_REG		0x088
/* H-Blanking Start/End */
#define	PLUM_VIDEO_PLHBEHSS_REG		0x08c
/* Horizontal # of pixel */
#define	PLUM_VIDEO_PLHPX_REG		0x090
/* Vertical Total */
#define	PLUM_VIDEO_PLVT_REG		0x094
/* Vertical Display Start */
#define	PLUM_VIDEO_PLVDS_REG		0x098
/* V-Sync Start/End */
#define	PLUM_VIDEO_PLVSEVSS_REG		0x09c
/* V-Blankng Start/End */
#define	PLUM_VIDEO_PLVBEVBS_REG		0x0a0
/* Current Line # */
#define	PLUM_VIDEO_PLCLN_REG		0x0a8
/* Interrupt Line # */
#define	PLUM_VIDEO_PLILN_REG		0x0ac
/* Mode */
#define	PLUM_VIDEO_PLMOD_REG		0x0b0
/* LCD controller test */
#define	PLUM_VIDEO_PLTST_REG		0x0bc

/*
 *	LCD Graphics Register
 */
/* Double Buffer Select */
#define	PLUM_VIDEO_PLBSL_REG		0x0c0
/* Graphics Display Start Address */
#define	PLUM_VIDEO_PLDSA0_REG		0x0c4
#define	PLUM_VIDEO_PLDSA1_REG		0x0c8
/* VRAM Pitch 1 */
#define	PLUM_VIDEO_PLPIT1_REG		0x0cc
/* VRAM Pitch 2 */
#define	PLUM_VIDEO_PLPIT2_REG		0x0d0
/* VRAM Offset */
#define	PLUM_VIDEO_PLOFS_REG		0x0d4
/* VRAM Lower Screen Address offset */
#define	PLUM_VIDEO_PLLSA_REG		0x0d8
/* Graphics Mode */
#define	PLUM_VIDEO_PLGMD_REG		0x0dc

#define PLUM_VIDEO_PLGMD_MASK		0x3
#define PLUM_VIDEO_PLGMD_DISABLE	0x0
#define PLUM_VIDEO_PLGMD_8BPP		0x1
#define PLUM_VIDEO_PLGMD_16BPP		0x2
/*
 *	CRT Timing Register
 */
/* notyet */
/*
 *	CRT Graphics Register
 */
/* notyet */
