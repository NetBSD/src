/*	$NetBSD: r128fbreg.h,v 1.1.28.1 2010/10/09 03:32:22 yamt Exp $	*/

/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Rickard E. Faith <faith@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 */

/*
 * register definitions for ATI Rage 128 graphics controllers
 * mostly from XFree86's ati driver
 */
 

#ifndef R128FB_REG_H
#define R128FB_REG_H

/* RAMDAC */
#define R128_PALETTE_DATA		0x00b4
#define R128_PALETTE_INDEX		0x00b0

/* flat panel registers */
#define R128_FP_PANEL_CNTL		0x0288
	#define FPCNT_DIGON		  0x00000001	/* FP dig. voltage */
	#define FPCNT_BACKLIGHT_ON	  0x00000002
	#define FPCNT_BL_MODULATION_ON	  0x00000004
	#define FPCNT_BL_CLK_SEL	  0x00000008	/* 1 - divide by 3 */
	#define FPCNT_MONID_EN		  0x00000010	/* use MONID pins for
							   backlight control */
	#define FPCNT_FPENABLE_POL	  0x00000020	/* 1 - active low */
	#define FPCNT_LEVEL_MASK	  0x0000ff00
	#define FPCNT_LEVEL_SHIFT	  8

#define R128_LVDS_GEN_CNTL                0x02d0
#       define R128_LVDS_ON               (1   <<  0)
#       define R128_LVDS_DISPLAY_DIS      (1   <<  1)
#       define R128_LVDS_EN               (1   <<  7)
#       define R128_LVDS_DIGON            (1   << 18)
#       define R128_LVDS_BLON             (1   << 19)
#       define R128_LVDS_SEL_CRTC2        (1   << 23)
#       define R128_HSYNC_DELAY_SHIFT     28
#       define R128_HSYNC_DELAY_MASK      (0xf << 28)
#	define R128_LEVEL_MASK	          0x0000ff00
#	define R128_LEVEL_SHIFT	          8

/* drawing engine */
#define R128_PC_NGUI_CTLSTAT              0x0184
#       define R128_PC_FLUSH_GUI          (3 << 0)
#       define R128_PC_RI_GUI             (1 << 2)
#       define R128_PC_FLUSH_ALL          0x00ff
#       define R128_PC_BUSY               (1 << 31)

#define R128_CRTC_OFFSET                  0x0224

#define R128_DST_OFFSET                   0x1404
#define R128_DST_PITCH                    0x1408

#define R128_DP_GUI_MASTER_CNTL           0x146c
#       define R128_GMC_SRC_PITCH_OFFSET_CNTL (1    <<  0)
#       define R128_GMC_DST_PITCH_OFFSET_CNTL (1    <<  1)
#       define R128_GMC_SRC_CLIPPING          (1    <<  2)
#       define R128_GMC_DST_CLIPPING          (1    <<  3)
#       define R128_GMC_BRUSH_DATATYPE_MASK   (0x0f <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_BG  (0    <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_LA  (1    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_BG  (4    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_LA  (5    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_BG (6    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_LA (7    <<  4)
#       define R128_GMC_BRUSH_32x32_MONO_FG_BG (8    <<  4)
#       define R128_GMC_BRUSH_32x32_MONO_FG_LA (9    <<  4)
#       define R128_GMC_BRUSH_8x8_COLOR       (10   <<  4)
#       define R128_GMC_BRUSH_1X8_COLOR       (12   <<  4)
#       define R128_GMC_BRUSH_SOLID_COLOR     (13   <<  4)
#       define R128_GMC_BRUSH_NONE            (15   <<  4)
#       define R128_GMC_DST_8BPP_CI           (2    <<  8)
#       define R128_GMC_DST_15BPP             (3    <<  8)
#       define R128_GMC_DST_16BPP             (4    <<  8)
#       define R128_GMC_DST_24BPP             (5    <<  8)
#       define R128_GMC_DST_32BPP             (6    <<  8)
#       define R128_GMC_DST_8BPP_RGB          (7    <<  8)
#       define R128_GMC_DST_Y8                (8    <<  8)
#       define R128_GMC_DST_RGB8              (9    <<  8)
#       define R128_GMC_DST_VYUY              (11   <<  8)
#       define R128_GMC_DST_YVYU              (12   <<  8)
#       define R128_GMC_DST_AYUV444           (14   <<  8)
#       define R128_GMC_DST_ARGB4444          (15   <<  8)
#       define R128_GMC_DST_DATATYPE_MASK     (0x0f <<  8)
#       define R128_GMC_DST_DATATYPE_SHIFT    8
#       define R128_GMC_SRC_DATATYPE_MASK       (3    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_BG (0    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_LA (1    << 12)
#       define R128_GMC_SRC_DATATYPE_COLOR      (3    << 12)
#       define R128_GMC_BYTE_PIX_ORDER        (1    << 14)
#       define R128_GMC_BYTE_MSB_TO_LSB       (0    << 14)
#       define R128_GMC_BYTE_LSB_TO_MSB       (1    << 14)
#       define R128_GMC_CONVERSION_TEMP       (1    << 15)
#       define R128_GMC_CONVERSION_TEMP_6500  (0    << 15)
#       define R128_GMC_CONVERSION_TEMP_9300  (1    << 15)
#       define R128_GMC_ROP3_MASK             (0xff << 16)
#       define R128_DP_SRC_SOURCE_MASK        (7    << 24)
#       define R128_DP_SRC_SOURCE_MEMORY      (2    << 24)
#       define R128_DP_SRC_SOURCE_HOST_DATA   (3    << 24)
#       define R128_DP_SRC_SOURCE_HOST_ALIGN  (4    << 24)
#       define R128_GMC_3D_FCN_EN             (1    << 27)
#       define R128_GMC_CLR_CMP_CNTL_DIS      (1    << 28)
#       define R128_GMC_AUX_CLIP_DIS          (1    << 29)
#       define R128_GMC_WR_MSK_DIS            (1    << 30)
#       define R128_GMC_LD_BRUSH_Y_X          (1    << 31)
#       define R128_ROP3_ZERO             0x00000000
#       define R128_ROP3_DSa              0x00880000
#       define R128_ROP3_SDna             0x00440000
#       define R128_ROP3_S                0x00cc0000
#       define R128_ROP3_DSna             0x00220000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DSx              0x00660000
#       define R128_ROP3_DSo              0x00ee0000
#       define R128_ROP3_DSon             0x00110000
#       define R128_ROP3_DSxn             0x00990000
#       define R128_ROP3_Dn               0x00550000
#       define R128_ROP3_SDno             0x00dd0000
#       define R128_ROP3_Sn               0x00330000
#       define R128_ROP3_DSno             0x00bb0000
#       define R128_ROP3_DSan             0x00770000
#       define R128_ROP3_ONE              0x00ff0000
#       define R128_ROP3_DPa              0x00a00000
#       define R128_ROP3_PDna             0x00500000
#       define R128_ROP3_P                0x00f00000
#       define R128_ROP3_DPna             0x000a0000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DPx              0x005a0000
#       define R128_ROP3_DPo              0x00fa0000
#       define R128_ROP3_DPon             0x00050000
#       define R128_ROP3_PDxn             0x00a50000
#       define R128_ROP3_PDno             0x00f50000
#       define R128_ROP3_Pn               0x000f0000
#       define R128_ROP3_DPno             0x00af0000
#       define R128_ROP3_DPan             0x005f0000

#define R128_DP_BRUSH_BKGD_CLR            0x1478
#define R128_DP_BRUSH_FRGD_CLR            0x147c
#define R128_SRC_X_Y                      0x1590
#define R128_DST_X_Y                      0x1594
#define R128_DST_WIDTH_HEIGHT             0x1598

#define R128_SRC_OFFSET                   0x15ac
#define R128_SRC_PITCH                    0x15b0

#define R128_AUX_SC_CNTL                  0x1660
#       define R128_AUX1_SC_EN            (1 << 0)
#       define R128_AUX1_SC_MODE_OR       (0 << 1)
#       define R128_AUX1_SC_MODE_NAND     (1 << 1)
#       define R128_AUX2_SC_EN            (1 << 2)
#       define R128_AUX2_SC_MODE_OR       (0 << 3)
#       define R128_AUX2_SC_MODE_NAND     (1 << 3)
#       define R128_AUX3_SC_EN            (1 << 4)
#       define R128_AUX3_SC_MODE_OR       (0 << 5)
#       define R128_AUX3_SC_MODE_NAND     (1 << 5)

#define R128_DP_CNTL                      0x16c0
#       define R128_DST_X_LEFT_TO_RIGHT   (1 <<  0)
#       define R128_DST_Y_TOP_TO_BOTTOM   (1 <<  1)

#define R128_DP_DATATYPE                  0x16c4
#       define R128_HOST_BIG_ENDIAN_EN    (1 << 29)

#define R128_DP_MIX                       0x16c8
#	define R128_MIX_SRC_VRAM		  (2 << 8)
#	define R128_MIX_SRC_HOSTDATA		  (3 << 8)
#	define R128_MIX_SRC_HOST_BYTEALIGN	  (4 << 8)
#	define R128_MIX_SRC_ROP3_MASK		  (0xff << 16)

#define R128_DP_WRITE_MASK                0x16cc
#define R128_DP_SRC_BKGD_CLR              0x15dc
#define R128_DP_SRC_FRGD_CLR              0x15d8

#define R128_DP_CNTL_XDIR_YDIR_YMAJOR     0x16d0
#       define R128_DST_Y_MAJOR             (1 <<  2)
#       define R128_DST_Y_DIR_TOP_TO_BOTTOM (1 << 15)
#       define R128_DST_X_DIR_LEFT_TO_RIGHT (1 << 31)

#define R128_DEFAULT_OFFSET               0x16e0
#define R128_DEFAULT_PITCH                0x16e4
#define R128_DEFAULT_SC_BOTTOM_RIGHT      0x16e8
#       define R128_DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#       define R128_DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)

/* scissor registers */
#define R128_SC_BOTTOM                    0x164c
#define R128_SC_BOTTOM_RIGHT              0x16f0
#define R128_SC_BOTTOM_RIGHT_C            0x1c8c
#define R128_SC_LEFT                      0x1640
#define R128_SC_RIGHT                     0x1644
#define R128_SC_TOP                       0x1648
#define R128_SC_TOP_LEFT                  0x16ec
#define R128_SC_TOP_LEFT_C                0x1c88

#define R128_GUI_STAT                     0x1740
#       define R128_GUI_FIFOCNT_MASK      0x0fff
#       define R128_GUI_ACTIVE            (1 << 31)

#define R128_HOST_DATA0                   0x17c0
#define R128_HOST_DATA1                   0x17c4
#define R128_HOST_DATA2                   0x17c8
#define R128_HOST_DATA3                   0x17cc
#define R128_HOST_DATA4                   0x17d0
#define R128_HOST_DATA5                   0x17d4
#define R128_HOST_DATA6                   0x17d8
#define R128_HOST_DATA7                   0x17dc

#endif /* R128FB_REG_H */
