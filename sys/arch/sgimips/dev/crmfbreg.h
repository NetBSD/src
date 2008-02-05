/* $NetBSD: crmfbreg.h,v 1.4 2008/02/05 21:39:25 macallan Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * SGI-CRM (O2) Framebuffer driver, register definitions
 */

#ifndef CRMFBREG_H
#define CRMFBREG_H

#define CRMFB_DOTCLOCK		0x00000004
#define		CRMFB_DOTCLOCK_CLKRUN_SHIFT	20
#define CRMFB_VT_XY		0x00010000
#define		CRMFB_VT_XY_FREEZE_SHIFT	31
#define	CRMFB_VT_FLAGS		0x00010018
#define		CRMFB_VT_FLAGS_SYNC_LOW_MSB	5
#define		CRMFB_VT_FLAGS_SYNC_LOW_LSB	5
#define CRMFB_VT_INTR01		0x00010020
#define		CRMFB_VT_INTR01_EN		0xffffffff
#define CRMFB_VT_INTR23		0x00010024
#define		CRMFB_VT_INTR23_EN		0xffffffff
#define CRMFB_VT_VPIX_EN	0x00010038
#define		CRMFB_VT_VPIX_EN_OFF_SHIFT	0
#define CRMFB_VT_HCMAP		0x0001003c
#define		CRMFB_VT_HCMAP_ON_SHIFT		12
#define CRMFB_VT_VCMAP		0x00010040
#define		CRMFB_VT_VCMAP_ON_SHIFT		12
#define CRMFB_OVR_WIDTH_TILE	0x00020000
#define CRMFB_OVR_CONTROL	0x00020008
#define		CRMFB_OVR_CONTROL_DMAEN_SHIFT	0
#define CRMFB_FRM_TILESIZE	0x00030000
#define		CRMFB_FRM_TILESIZE_RHS_SHIFT	0
#define		CRMFB_FRM_TILESIZE_WIDTH_SHIFT	5
#define		CRMFB_FRM_TILESIZE_DEPTH_SHIFT	13
#define			CRMFB_FRM_TILESIZE_DEPTH_8	0
#define			CRMFB_FRM_TILESIZE_DEPTH_16	1
#define			CRMFB_FRM_TILESIZE_DEPTH_32	2
#define		CRMFB_FRM_TILESIZE_FIFOR_SHIFT	15
#define CRMFB_FRM_PIXSIZE	0x00030004
#define		CRMFB_FRM_PIXSIZE_HEIGHT_SHIFT	16
#define CRMFB_FRM_CONTROL	0x0003000c
#define		CRMFB_FRM_CONTROL_DMAEN_SHIFT	0
#define		CRMFB_FRM_CONTROL_LINEAR_SHIFT	1
#define		CRMFB_FRM_CONTROL_TILEPTR_SHIFT	9
#define CRMFB_DID_CONTROL	0x00040004
#define		CRMFB_DID_CONTROL_DMAEN_SHIFT	0
#define CRMFB_MODE		0x00048000
#define		CRMFB_MODE_TYP_SHIFT		2
#define			CRMFB_MODE_TYP_I8	0
#define			CRMFB_MODE_TYP_ARGB5	4
#define			CRMFB_MODE_TYP_RGB8	5
#define		CRMFB_MODE_BUF_SHIFT		0
#define			CRMFB_MODE_BUF_BOTH	3
#define CRMFB_CMAP		0x00050000
#define CRMFB_CMAP_FIFO		0x00058000
#define CRMFB_GMAP		0x00060000
#define CRMFB_CURSOR_POS	0x00070000
/*
 * upper 16 bit are Y, lower 16 bit are X - both signed so there's no need for
 * a hotspot register
 */
#define CRMFB_CURSOR_CONTROL	0x00070004
	#define CRMFB_CURSOR_ON		0x00000001
#define CRMFB_CURSOR_CMAP0	0x00070008
#define CRMFB_CURSOR_CMAP1	0x0007000c
#define CRMFB_CURSOR_CMAP2	0x00070010
#define CRMFB_CURSOR_BITMAP	0x00078000
/* two bit deep cursor image, zero is transparent */

/* rendering engine registers */
#define CRIME_RE_TLB_A		0x1000
#define CRIME_RE_TLB_B		0x1200
#define CRIME_RE_TLB_C		0x1400
#define CRIME_RE_TEX		0x1600
#define CRIME_RE_CLIP_IDS	0x16e0
#define CRIME_RE_LINEAR_A	0x1700
#define CRIME_RE_LINEAR_B	0x1780

/* memory transfer engine from 0x3000*/
#define CRIME_MTE_MODE		0x3000
#define CRIME_MTE_BYTEMASK	0x3008
#define CRIME_MTE_STIPPLEMASK	0x3010
#define CRIME_MTE_BG		0x3018
#define CRIME_MTE_SRC0		0x3020	/* start */
#define CRIME_MTE_SRC1		0x3028	/* end */
#define CRIME_MTE_DST0		0x3030	/* start */
#define CRIME_MTE_DST1		0x3038	/* end */
#define CRIME_MTE_SRC_STRIDE	0x3040
#define CRIME_MTE_DST_STRIDE	0x3048
#define CRIME_MTE_NULL		0x3070
#define CRIME_MTE_FLUSH		0x3078

/* CRIME_MTE_MODE */
#define MTE_MODE_DST_ECC	0x00000001	/* enable ECC in DST */
#define MTE_MODE_SRC_ECC	0x00000002	/* enable ESS in SRC */
#define MTE_MODE_DST_BUF_MASK	0x0000001c
	#define MTE_TLB_A	0
	#define MTE_TLB_B	1
	#define MTE_TLB_C	2
	#define MTE_TLB_TEX	3
	#define MTE_TLB_LIN_A	4
	#define MTE_TLB_LIN_B	5
	#define MTE_TLB_CLIP	6
	#define MTE_DST_TLB_SHIFT 2
#define MTE_MODE_SRC_BUF_MASK	0x000000e0
	#define MTE_SRC_TLB_SHIFT 5
#define MTE_MODE_DEPTH_MASK	0x00000300
	#define MTE_DEPTH_8	0
	#define MTE_DEPTH_16	1
	#define MTE_DEPTH_32	2
	#define MTE_DEPTH_SHIFT 8
#define MTE_MODE_STIPPLE	0x00000400
#define MTE_MODE_COPY		0x00000800	/* 1 - copy, 0 - clear dst */

/* drawing engine from 0x2000 */
#define CRIME_DE_MODE_SRC	0x2000
#define CRIME_DE_MODE_DST	0x2008
#define CRIME_DE_CLIPMODE	0x2010
#define CRIME_DE_DRAWMODE	0x2018
#define CRIME_DE_SCRMASK0	0x2020
#define CRIME_DE_SCRMASK1	0x2028
#define CRIME_DE_SCRMASK2	0x2030
#define CRIME_DE_SCRMASK3	0x2038
#define CRIME_DE_SCRMASK4	0x2040
#define CRIME_DE_SCISSOR	0x2048
#define CRIME_DE_WINOFFSET_SRC	0x2050	/* x in upper, y in lower 16 bit */
#define CRIME_DE_WINOFFSET_DST	0x2058
#define CRIME_DE_PRIMITIVE	0x2060
#define CRIME_DE_X_VERTEX_0	0x2070
#define CRIME_DE_X_VERTEX_1	0x2074
#define CRIME_DE_X_VERTEX_2	0x2078
#define CRIME_DE_GL_VERTEX_0_X	0x2080
#define CRIME_DE_GL_VERTEX_0_Y	0x2084
#define CRIME_DE_GL_VERTEX_1_X	0x2088
#define CRIME_DE_GL_VERTEX_1_Y	0x208c
#define CRIME_DE_GL_VERTEX_2_X	0x2090
#define CRIME_DE_GL_VERTEX_2_Y	0x2094
#define CRIME_DE_XFER_ADDR_SRC	0x20a0
#define CRIME_DE_XFER_STEP_X	0x20a8
#define CRIME_DE_XFER_STEP_Y	0x20ac
#define CRIME_DE_XFER_DST_ADDR	0x20b0
#define CRIME_DE_XFER_DST_STRD	0x20b4
#define CRIME_DE_STIPPLE_MODE	0x20c0
#define CRIME_DE_STIPPLE_PAT	0x20c4
#define CRIME_DE_FG		0x20d0
#define CRIME_DE_BG		0x20d8

#define CRIME_DE_ROP		0x21b0
#define CRIME_DE_PLANEMASK	0x21b8

#define CRIME_DE_NULL		0x21f0
#define CRIME_DE_FLUSH		0x21f8

#define CRIME_DE_START		0x0800	/* OR this to a register address in
					 * order to start a command */

/* CRIME_DE_MODE_* */
#define DE_MODE_TLB_A		0x00000000
#define DE_MODE_TLB_B		0x00000400
#define DE_MODE_TLB_C		0x00000800
#define DE_MODE_LIN_A		0x00001000
#define DE_MODE_LIN_B		0x00001400
#define DE_MODE_BUFDEPTH_8	0x00000000
#define DE_MODE_BUFDEPTH_16	0x00000100
#define DE_MODE_BUFDEPTH_32	0x00000200
#define DE_MODE_TYPE_CI		0x00000000
#define DE_MODE_TYPE_RGB	0x00000010
#define DE_MODE_TYPE_RGBA	0x00000020
#define DE_MODE_TYPE_ABGR	0x00000030
#define DE_MODE_TYPE_YCRCB	0x000000f0
#define DE_MODE_PIXDEPTH_8	0x00000000
#define DE_MODE_PIXDEPTH_16	0x00000004
#define DE_MODE_PIXDEPTH_32	0x00000008
#define DE_MODE_DOUBLE_PIX	0x00000002
#define DE_MODE_DOUBLE_SELECT	0x00000001

/* clip mode */
#define DE_CLIPMODE_ENABLE	0x00000800

/* draw mode */
#define DE_DRAWMODE_NO_CONF	0x00800000	/* disable coherency testing */
#define DE_DRAWMODE_X11		0x00000000
#define DE_DRAWMODE_GL		0x00400000
#define DE_DRAWMODE_XFER_EN	0x00200000
#define DE_DRAWMODE_SCISSOR_EN	0x00100000
#define DE_DRAWMODE_LINE_STIP	0x00080000
#define DE_DRAWMODE_POLY_STIP	0x00040000
#define DE_DRAWMODE_OPAQUE_STIP	0x00020000
#define DE_DRAWMODE_SHADE	0x00010000	/* smooth shading enable */
#define DE_DRAWMODE_TEXTURE	0x00008000
#define DE_DRAWMODE_FOG		0x00004000
#define DE_DRAWMODE_COVERAGE	0x00002000
#define DE_DRAWMODE_LINE_AA	0x00001000
#define DE_DRAWMODE_ALPHA_TEST	0x00000800
#define DE_DRAWMODE_ALPHA_BLEND	0x00000400
#define DE_DRAWMODE_ROP		0x00000200
#define DE_DRAWMODE_DITHER	0x00000100
#define DE_DRAWMODE_PLANEMASK	0x00000080
#define DE_DRAWMODE_BYTEMASK	0x00000078
#define DE_DRAWMODE_DEPTH_TEST	0x00000004
#define DE_DRAWMODE_DEPTH_MASK	0x00000002
#define DE_DRAWMODE_STENCIL	0x00000001

/* primitive */
#define DE_PRIM_POINT		0x00000000
#define DE_PRIM_LINE		0x01000000
#define DE_PRIM_TRIANGLE	0x02000000
#define DE_PRIM_RECTANGLE	0x03000000
#define DE_PRIM_LINE_SKIP_END	0x00040000
#define DE_PRIM_LR		0x00000000	/* left to right */
#define DE_PRIM_RL		0x00010000	/* right to left */
#define DE_PRIM_BT		0x00000000	/* bottom to top */
#define DE_PRIM_TB		0x00020000	/* top to bottom */
#define DE_PRIM_LINE_WIDTH_MASK	0x0000ffff	/* in half pixels */

/* status register */
#define CRIME_DE_STATUS		0x4000
#define CRIME_DE_IDLE		0x10000000

#endif /* CRMFBREG_H */
