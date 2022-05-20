/*	$NetBSD: ffbreg.h,v 1.12 2022/05/20 19:34:22 andvar Exp $	*/
/*	$OpenBSD: creatorreg.h,v 1.5 2002/07/29 06:21:45 jason Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
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
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FFB_REG_H
#define FFB_REG_H

/* Number of register sets */
#define	FFB_NREGS		24

/* Register set numbers */
#define	FFB_REG_PROM		0
#define	FFB_REG_DAC		1
#define	FFB_REG_FBC		2
#define	FFB_REG_DFB8R		3
#define	FFB_REG_DFB8G		4
#define	FFB_REG_DFB8B		5
#define	FFB_REG_DFB8X		6
#define	FFB_REG_DFB24		7
#define	FFB_REG_DFB32		8
#define	FFB_REG_SFB8R		9
#define	FFB_REG_SFB8G		10
#define	FFB_REG_SFB8B		11
#define	FFB_REG_SFB8X		12
#define	FFB_REG_SFB32		13
#define	FFB_REG_SFB64		14
#define	FFB_REG_DFB422A		15

#define	FFB_DAC_TYPE		0x0
#define	FFB_DAC_VALUE		0x4
#define	FFB_DAC_TYPE2		0x8
#define	FFB_DAC_VALUE2		0xc

/* DAC "TYPE" commands (registers) */
#define	FFB_DAC_PLL_CTRL	0x0000	/* PLL control (frequency) */
#define	FFB_DAC_PIX_FMT		0x1000	/* Pixel format control */
#define	FFB_DAC_USR_CTRL	0x1001	/* user control */
#define	FFB_DAC_SCMAP		0x2000	/* set (load) cmap */
#define	FFB_DAC_DAC_CTRL	0x5001	/* DAC control */
#define	FFB_DAC_TGC		0x6000	/* timing generator control */
#define	FFB_DAC_VBE		0x6001	/* vertical blank end */
#define	FFB_DAC_VBS		0x6002	/* vertical blank start */
#define	FFB_DAC_VSE		0x6003	/* vertical sync end */
#define	FFB_DAC_VSS		0x6004	/* vertical sync start */
#define	FFB_DAC_HRE		0x6005	/* horizontal serration end */
#define	FFB_DAC_HBE		0x6006	/* horizontal blank end */
#define	FFB_DAC_HBS		0x6007	/* horizontal blank start */
#define	FFB_DAC_HSE		0x6008	/* horizontal sync end */
#define	FFB_DAC_HSS		0x6009	/* horizontal sync start */
#define	FFB_DAC_HCE		0x600a	/* horiz. serial clock enable end */
#define	FFB_DAC_HCS		0x600b	/* horiz. serial clock enable start */
#define	FFB_DAC_EPE		0x600c	/* equalisation pulse end */
#define	FFB_DAC_EIE		0x600d	/* equalisation interval end */
#define	FFB_DAC_EIS		0x600e	/* equalisation interval start */
#define	FFB_DAC_TVC		0x600f	/* timing generator vertical counter */
#define	FFB_DAC_THC		0x6010	/* timing generator horiz. counter */
#define	FFB_DAC_DEVID		0x8000	/* DAC device ID (version) */
#define	FFB_DAC_CFG_MPDATA	0x8001	/* monitor serial port data */
#define	FFB_DAC_CFG_MPSENSE	0x8002	/* monitor serial port sense */

/* 0x1000 pixel format control */
#define	FFB_DAC_PIX_FMT_421		0x02	/* 4/2:1 */
#define	FFB_DAC_PIX_FMT_821		0x03	/* 8/2:1 */

/* 0x1001 user control */
#define	FFB_DAC_USR_CTRL_BLANK		0x02	/* asynchronous blank */
#define	FFB_DAC_USR_CTRL_DOUBLE		0x04	/* double-buffer enable */
#define	FFB_DAC_USR_CTRL_OVERLAY	0x08	/* transparent overlay enable */
#define	FFB_DAC_USR_CTRL_WMODE_C	0x00	/* window mode combined */
#define	FFB_DAC_USR_CTRL_WMODE_S4	0x10	/* window mode separate 4 */
#define	FFB_DAC_USR_CTRL_WMODE_S8	0x20	/* window mode separate 8 */

/* 0x5001 DAC control */
#define	FFB_DAC_DAC_CTRL_SYNC_G		0x0020	/* enable sync on green */
#define FFB_DAC_DAC_CTRL_PED_ENABLE	0x0040	/* enable pedestal */
#define FFB_DAC_DAC_CTRL_VSYNC_DIS	0x0080	/* disable vsync pin */
#define FFB_DAC_DAC_CTRL_POS_VSYNC	0x0100	/* enable pos. vsync */

/* 0x6000 timing generator control */
#define	FFB_DAC_TGC_VIDEO_ENABLE	0x01	/* enable DAC outputs */
#define	FFB_DAC_TGC_TIMING_ENABLE	0x02	/* enable timing generator */
#define	FFB_DAC_TGC_HSYNC_DISABLE	0x04	/* disable hsync on csync */
#define	FFB_DAC_TGC_VSYNC_DISABLE	0x08	/* disable vsync on csync */
#define	FFB_DAC_TGC_EQUAL_DISABLE	0x10	/* disable equalisation */
#define	FFB_DAC_TGC_MASTER_ENABLE	0x20	/* enable master mode */
#define	FFB_DAC_TGC_ILACE_ENABLE	0x40	/* enable interlaced mode */

/* 0x8001 monitor serial port data */
#define	FFB_DAC_CFG_MPDATA_SCL		0x01	/* SCL Data */
#define	FFB_DAC_CFG_MPDATA_SDA		0x02	/* SDA Data */

/* 0x8002 monitor serial port sense */
#define	FFB_DAC_CFG_MPSENSE_SCL		0x01	/* SCL Sense */
#define	FFB_DAC_CFG_MPSENSE_SDA		0x02	/* SDA Sense */

/* DAC "TYPE2" commands */
#define	FFB_DAC_CURSENAB	0x100	/* cursor enable */
#define	FFB_DAC_CURSECMAP	0x102	/* set cursor colormap */
#define	FFB_DAC_CURSEPOS	0x104	/* set cursor position */

#define	FFB_FBC_ALPHA		0x00c
#define	FFB_FBC_RED		0x010
#define	FFB_FBC_GREEN		0x014
#define	FFB_FBC_BLUE		0x018
#define	FFB_FBC_DEPTH		0x01c
#define	FFB_FBC_Y		0x020
#define	FFB_FBC_X		0x024
#define	FFB_FBC_RYF		0x030
#define	FFB_FBC_RXF		0x034
#define	FFB_FBC_DMYF		0x040
#define	FFB_FBC_DMXF		0x044
#define	FFB_FBC_EBYI		0x050
#define	FFB_FBC_EBXI		0x054
#define	FFB_FBC_BY		0x060
#define	FFB_FBC_BX		0x064
#define	FFB_FBC_DY		0x068
#define	FFB_FBC_DX		0x06c
#define	FFB_FBC_BH		0x070
#define	FFB_FBC_BW		0x074
#define	FFB_FBC_SUVTX		0x100
#define	FFB_FBC_PPC		0x200	/* pixel processor control */
#define	FFB_FBC_WID		0x204	/* current WID */
#define	FFB_FBC_FG		0x208
#define	FFB_FBC_BG		0x20c
#define	FFB_FBC_CONSTY		0x210
#define	FFB_FBC_CONSTZ		0x214
#define	FFB_FBC_XCLIP		0x218
#define	FFB_FBC_DCSS		0x21c
#define	FFB_FBC_VCLIPMIN	0x220	/* Viewclip XY Min Bounds */
#define	FFB_FBC_VCLIPMAX	0x224
#define	FFB_FBC_VCLIPZMIN	0x228
#define	FFB_FBC_VCLIPZMAX	0x22c
#define	FFB_FBC_DCSF		0x230
#define	FFB_FBC_DCSB		0x234
#define	FFB_FBC_DCZF		0x238
#define	FFB_FBC_DCZB		0x23c
#define	FFB_FBC_BLENDC		0x244
#define	FFB_FBC_BLENDC1		0x248
#define	FFB_FBC_BLENDC2		0x24c
#define	FFB_FBC_FBRAMITC	0x250
#define	FFB_FBC_FBC		0x254	/* Frame Buffer Control	*/
#define	FFB_FBC_ROP		0x258	/* Raster OPeration */
#define	FFB_FBC_CMP		0x25c	/* Frame Buffer Compare */
#define	FFB_FBC_MATCHAB		0x260	/* Buffer AB Match Mask	*/
#define	FFB_FBC_MATCHC		0x264
#define	FFB_FBC_MAGNAB		0x268	/* Buffer AB Magnitude Mask */
#define	FFB_FBC_MAGNC		0x26c
#define	FFB_FBC_FBCFG0		0x270
#define	FFB_FBC_FBCFG1		0x274
#define	FFB_FBC_FBCFG2		0x278
#define	FFB_FBC_FBCFG3		0x27c
#define	FFB_FBC_PPCFG		0x280
#define	FFB_FBC_PICK		0x284
#define	FFB_FBC_FILLMODE	0x288
#define	FFB_FBC_FBRAMWAC	0x28c	/* FB RAM Write Address Control */
#define	FFB_FBC_PMASK		0x290	/* RGB Plane Mask */
#define	FFB_FBC_XPMASK		0x294	/* X PlaneMask */
#define	FFB_FBC_YPMASK		0x298
#define	FFB_FBC_ZPMASK		0x29c
#define	FFB_FBC_CLIP0MIN	0x2a0	/* Auxiliary Viewport Clips */
#define	FFB_FBC_CLIP0MAX	0x2a4
#define	FFB_FBC_CLIP1MIN	0x2a8
#define	FFB_FBC_CLIP1MAX	0x2ac
#define	FFB_FBC_CLIP2MIN	0x2b0
#define	FFB_FBC_CLIP2MAX	0x2b4
#define	FFB_FBC_CLIP3MIN	0x2b8
#define	FFB_FBC_CLIP3MAX	0x2bc
#define	FFB_FBC_RAWBLEND2	0x2c0
#define	FFB_FBC_RAWPREBLEND	0x2c4
#define	FFB_FBC_RAWSTENCIL	0x2c8
#define	FFB_FBC_RAWSTENCILCTL	0x2cc
#define	FFB_FBC_THREEDRAM1	0x2d0
#define	FFB_FBC_THREEDRAM2	0x2d4
#define	FFB_FBC_PASSIN		0x2d8
#define	FFB_FBC_RAWCLRDEPTH	0x2dc
#define	FFB_FBC_RAWPMASK	0x2e0
#define	FFB_FBC_RAWCSRC		0x2e4
#define	FFB_FBC_RAWMATCH	0x2e8
#define	FFB_FBC_RAWMAGN		0x2ec
#define	FFB_FBC_RAWROPBLEND	0x2f0
#define	FFB_FBC_RAWCMP		0x2f4
#define	FFB_FBC_RAWWAC		0x2f8
#define	FFB_FBC_FBRAMID		0x2fc
#define	FFB_FBC_DRAWOP		0x300	/* Draw OPeration */
#define	FFB_FBC_FONTLPAT	0x30c	/* Line Pattern control */
#define	FFB_FBC_FONTXY		0x314	/* XY Font coordinate */
#define	FFB_FBC_FONTW		0x318	/* Font Width */
#define	FFB_FBC_FONTINC		0x31c	/* Font Increment */
#define	FFB_FBC_FONT		0x320
#define	FFB_FBC_BLEND2		0x330
#define	FFB_FBC_PREBLEND	0x334
#define	FFB_FBC_STENCIL		0x338
#define	FFB_FBC_STENCILCTL	0x33c
#define	FFB_FBC_DCSS1		0x350
#define	FFB_FBC_DCSS2		0x354
#define	FFB_FBC_DCSS3		0x358
#define	FFB_FBC_WIDPMASK	0x35c
#define	FFB_FBC_DCS2		0x360
#define	FFB_FBC_DCS3		0x364
#define	FFB_FBC_DCS4		0x368
#define	FFB_FBC_DCD2		0x370
#define	FFB_FBC_DCD3		0x374
#define	FFB_FBC_DCD4		0x378
#define	FFB_FBC_PATTERN		0x380
#define	FFB_FBC_DEVID		0x800
#define	FFB_FBC_UCSR		0x900	/* User Control & Status */
#define	FFB_FBC_MER		0x980
#define	FFB_FBC_RAMCNF0		0x10270	/* FBRAM Configuration 0 */
#define	FFB_FBC_RAMCNF1		0x10274	/* FBRAM Configuration 1 */
#define	FFB_FBC_RAMCNF2		0x10278	/* FBRAM Configuration 2 */
#define	FFB_FBC_RAMCNF3		0x1027c	/* FBRAM Configuration 3 */
#define	FFB_FBC_KCSR		0x10900	/* Kernel Control & Status */

#define	FFB_FBC_WB_A		0x20000000
#define	FFB_FBC_WB_B		0x40000000
#define FFB_FBC_WE_FORCEOFF	0x00100000
#define FFB_FBC_WE_FORCEON	0x00200000
#define	FFB_FBC_WM_COMBINED	0x00080000
#define	FFB_FBC_RB_A		0x00004000
#define	FFB_FBC_SB_BOTH		0x00003000
#define	FFB_FBC_ZE_OFF		0x00000400
#define	FFB_FBC_YE_OFF		0x00000100
#define	FFB_FBC_XE_ON		0x00000080
#define	FFB_FBC_XE_OFF		0x00000040
#define	FFB_FBC_RGBE_ON		0x0000002a
#define	FFB_FBC_RGBE_OFF	0x00000015
#define	FFB_FBC_RGBE_MASK	0x0000003f

#define	FBC_PPC_FW_DIS		0x00800000	/* force wid disable */
#define	FBC_PPC_FW_ENA		0x00c00000	/* force wid enable */
#define	FBC_PPC_ACE_DIS		0x00040000	/* aux clip disable */
#define	FBC_PPC_ACE_AUXSUB	0x00080000	/* aux clip add */
#define	FBC_PPC_ACE_AUXADD	0x000c0000	/* aux clip subtract */
#define	FBC_PPC_DCE_DIS		0x00020000	/* depth cue disable */
#define	FBC_PPC_DCE_ENA		0x00020000	/* depth cue enable */
#define	FBC_PPC_ABE_DIS		0x00008000	/* alpha blend disable */
#define	FBC_PPC_ABE_ENA		0x0000c000	/* alpha blend enable */
#define	FBC_PPC_VCE_DIS		0x00001000	/* view clip disable */
#define	FBC_PPC_VCE_2D		0x00002000	/* view clip 2d */
#define	FBC_PPC_VCE_3D		0x00003000	/* view clip 3d */
#define	FBC_PPC_APE_DIS		0x00000800	/* area pattern disable */
#define	FBC_PPC_APE_ENA		0x00000c00	/* area pattern enable */
#define	FBC_PPC_TBE_OPAQUE	0x00000200	/* opaque background */
#define	FBC_PPC_TBE_TRANSPAR	0x00000300	/* transparent background */
#define	FBC_PPC_ZS_VAR		0x00000080	/* z source ??? */
#define	FBC_PPC_ZS_CONST	0x000000c0	/* z source ??? */
#define	FBC_PPC_YS_VAR		0x00000020	/* y source ??? */
#define	FBC_PPC_YS_CONST	0x00000030	/* y source ??? */
#define	FBC_PPC_XS_WID		0x00000004	/* x source ??? */
#define	FBC_PPC_XS_VAR		0x00000008	/* x source ??? */
#define	FBC_PPC_XS_CONST	0x0000000c	/* x source ??? */
#define	FBC_PPC_CS_VAR		0x00000002	/* color source ??? */
#define	FBC_PPC_CS_CONST	0x00000003	/* color source ??? */

#define	FBC_ROP_NEW		0x83
#define	FBC_ROP_OLD		0x85
#define	FBC_ROP_INVERT		0x8a

#define	FBC_UCSR_FIFO_MASK	0x00000fff
#define	FBC_UCSR_FB_BUSY	0x01000000
#define	FBC_UCSR_RP_BUSY	0x02000000
#define	FBC_UCSR_READ_ERR	0x40000000
#define	FBC_UCSR_FIFO_OVFL	0x80000000

#define	FBC_DRAWOP_DOT		0x00
#define	FBC_DRAWOP_AADOT	0x01
#define	FBC_DRAWOP_BRLINECAP	0x02
#define	FBC_DRAWOP_BRLINEOPEN	0x03
#define	FBC_DRAWOP_DDLINE	0x04
#define	FBC_DRAWOP_AALINE	0x05
#define	FBC_DRAWOP_TRIANGLE	0x06
#define	FBC_DRAWOP_POLYGON	0x07
#define	FBC_DRAWOP_RECTANGLE	0x08
#define	FBC_DRAWOP_FASTFILL	0x09
#define	FBC_DRAWOP_BCOPY	0x0a	/* block copy: not implemented */
#define	FBC_DRAWOP_VSCROLL	0x0b	/* vertical scroll */

#define FBC_CFG0_RES_MASK	0x30	/* Resolution bits */
#define FBC_CFG0_STEREO		0x10	/* Stereo */
#define FBC_CFG0_SINGLE_BUF	0x20	/* Single buffer */
#define FBC_CFG0_DOUBLE_BUF	0x30	/* Double buffer */

/* Alpha Blend Control */
#define FFB_BLENDC_FORCE_ONE	0x00000010 /* Defines 0xff as 1.0 */
#define FFB_BLENDC_DF_MASK	0x0000000c /* Destination Frac Mask */
#define FFB_BLENDC_DF_ZERO	0x00000000 /* Destination Frac: 0.00 */
#define FFB_BLENDC_DF_ONE	0x00000004 /* Destination Frac: 1.00 */
#define FFB_BLENDC_DF_ONE_M_A	0x00000008 /* Destination Frac: 1.00 - Xsrc */
#define FFB_BLENDC_DF_A		0x0000000c /* Destination Frac: Xsrc */
#define FFB_BLENDC_SF_MASK	0x00000003 /* Source Frac Mask */
#define FFB_BLENDC_SF_ZERO	0x00000000 /* Source Frac: 0.00 */
#define FFB_BLENDC_SF_ONE	0x00000001 /* Source Frac: 1.00 */
#define FFB_BLENDC_SF_ONE_M_A	0x00000002 /* Source Frac: 1.00 - Xsrc */
#define FFB_BLENDC_SF_A		0x00000003 /* Source Frac: Xsrc */


#endif /* FFB_REG_H */
