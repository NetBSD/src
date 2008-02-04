/* $NetBSD: crmfbreg.h,v 1.1.14.4 2008/02/04 09:22:26 yamt Exp $ */

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
#define CROME_MTE_SOURCE0	0x3020
#define CRIME_MTE_SOURCE1	0x3028
#define CROME_MTE_DEST0		0x3030
#define CRIME_MTE_DEST1		0x3038
#define CRIME_MTE_SRC_STRIDE	0x3040
#define CRIME_MTE_DST_STRIDE	0x3048
#define CRIME_MTE_NULL		0x3070
#define CRIME_MTE_FLUSH		0x3078

#endif /* CRMFBREG_H */
