/*
 * Copyright (c) 2015 Naruaki Etomi
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
 * register definitions for Permedia 3 graphics controllers
 */


#ifndef PM3_REG_H
#define PM3_REG_H

#define PM3_EXT_CLOCK_FREQ 143180 /*in 100Hz units*/
#define PM3_VCO_FREQ_MIN 2000000 /*in 100Hz units*/
#define PM3_VCO_FREQ_MAX 6220000 /*in 100Hz units*/
#define PM3_INTREF_MIN 10000 /*in 100Hz units*/
#define PM3_INTREF_MAX 20000 /*in 100Hz units*/

/*
 * PM3 RAMDAC Indirect Commands
 */

#define PM3_RAMDAC_CMD_MISC_CONTROL            0x000
#define PM3_RAMDAC_CMD_SYNC_CONTROL            0x001
#define PM3_RAMDAC_CMD_DAC_CONTROL             0x002
#define PM3_RAMDAC_CMD_PIXEL_SIZE              0x003
#define PM3_RAMDAC_CMD_COLOR_FORMAT            0x004
#define PM3_RAMDAC_CMD_CLOCK0_PRE_SCALE        0x201
#define PM3_RAMDAC_CMD_CLOCK0_FEEDBACK_SCALE   0x202
#define PM3_RAMDAC_CMD_CLOCK0_POST_SCALE       0x203

/*
 * PM3 RAMDAC Indirect Command SYNC_CONTROL
 */

#define PM3_SC_HSYNC_ACTIVE_LOW                0x000
#define PM3_SC_HSYNC_ACTIVE_HIGH               0x001
#define PM3_SC_HSYNC_FORCE_ACTIVE              0x003
#define PM3_SC_HSYNC_FORCE_INACTIVE            0x004
#define PM3_SC_VSYNC_ACTIVE_LOW                0x000
#define PM3_SC_VSYNC_ACTIVE_HIGH               0x008
#define PM3_SC_VSYNC_FORCE_ACTIVE              0x018
#define PM3_SC_VSYNC_FORCE_INACTIVE            0x020

/*
 * PM3 RAMDAC Indirect Command DAC_CONTROL
 */

#define PM3_DC_NORMAL_POWER                    0x000
#define PM3_DC_LOW_POWER                       0x001
#define PM3_DC_SYNC_ON_GREEN_ENABLE            0x008
#define PM3_DC_SYNC_ON_GREEN_DISABLE           0x000
#define PM3_DC_BLANK_PEDESTAL_ENABLE           0x080
#define PM3_DC_BLANK_PEDESTAL_DISABLE          0x000

/*
 * PM3 RAMDAC Indirect Command PIXEL_SIZE
 */

#define PM3_DACPS_8BIT                         0x000
#define PM3_DACPS_16BIT                        0x001
#define PM3_DACPS_32BIT                        0x002
#define PM3_DACPS_24BIT                        0x004

/*
 * PM3 RAMDAC Indirect Command COLOR_FORMAT
 */

#define PM3_CF_VISUAL_256_COLOR                0x00e
#define PM3_CF_VISUAL_HIGH_COLOR               0x010
#define PM3_CF_VISUAL_TRUE_COLOR               0x000
#define PM3_CF_ORDER_BGR                       0x020
#define PM3_CF_ORDER_RGB                       0x000
#define PM3_CF_LINEAR_COLOR_EXT_ENABLE         0x040
#define PM3_CF_LINEAR_COLOR_EXT_DISABLE        0x000

/*
 * PM3 RAMDAC Indirect Command MISC_CONTROL
 */

#define PM3_MC_DAC_SIZE_8BIT                   0x001
#define PM3_MC_DAC_SIZE_6BIT                   0x000
#define PM3_MC_PIXEL_DOUBLE_ENABLE             0x002
#define PM3_MC_PIXEL_DOUBLE_DISABLE            0x000
#define PM3_MC_LAST_READ_ADDR_ENABLE           0x004
#define PM3_MC_LAST_READ_ADDR_DISABLE          0x000
#define PM3_MC_DIRECT_COLOR_ENABLE             0x008
#define PM3_MC_DIRECT_COLOR_DISABLE            0x000
#define PM3_MC_OVERLAY_ENABLE                  0x010
#define PM3_MC_OVERLAY_DISABLE                 0x000
#define PM3_MC_PIXEL_DB_ENABLE                 0x020
#define PM3_MC_PIXEL_DB_DISABLE                0x000
#define PM3_MC_STEREO_DB_ENABLE                0x040
#define PM3_MC_STEREO_DB_DISABLE               0x000

/*
 * PM3 Registers
 */

#define PM3_INPUT_FIFO_SPACE                   0x00000018
#define PM3_OUTPUT_FIFO_WORDS                  0x00000020
#define PM3_V_CLOCK_CTL                        0x00000040
#define PM3_APERTURE1_CONTROL                  0x00000050
#define PM3_APERTURE2_CONTROL                  0x00000058
#define              PM3_AP_MEMORY_BYTE_STANDARD                   0x00000000
#define              PM3_AP_MEMORY_BYTE_SWAPPED                    0x00000001
#define PM3_FIFODISCONNECT                     0x00000068
#define PM3_CHIP_CONFIG                        0x00000070
#define PM3_BYAPERTURE1MODE                    0x00000300
#define PM3_BYAPERTURE2MODE                    0x00000328
#define              PM3_BYAPERTUREMODE_PIXELSIZE_8BIT             0x00000000
#define              PM3_BYAPERTUREMODE_PIXELSIZE_16BIT            0x00000020
#define              PM3_BYAPERTUREMODE_PIXELSIZE_32BIT            0x00000040

#define PM3_BYPASS_MASK                        0x00001008
#define PM3_LOCALMEMCAPS                       0x00001018
#define PM3_LOCALMEMTIMINGS                    0x00001020
#define PM3_LOCALMEMCONTROL                    0x00001028

#define PM3_OUTPUT_FIFO                        0x00002000

#define PM3_SCREEN_BASE                        0x00003000
#define PM3_SCREEN_STRIDE                      0x00003008
#define PM3_HORIZ_TOTAL                        0x00003010
#define PM3_HORIZ_GATE_END                     0x00003018
#define PM3_HORIZ_BLANK_END                    0x00003020
#define PM3_HORIZ_SYNC_START                   0x00003028
#define PM3_HORIZ_SYNC_END                     0x00003030
#define PM3_VERT_TOTAL                         0x00003038
#define PM3_VERT_BLANK_END                     0x00003040
#define PM3_VERT_SYNC_START                    0x00003048
#define PM3_VERT_SYNC_END                      0x00003050
#define PM3_VIDEO_CONTROL                      0x00003058
#define              M3_VC_DISABLE                                 0x00000000
#define              PM3_VC_ENABLE                                 0x00000001
#define              PM3_VC_BL_ACTIVE_HIGH                         0x00000000
#define              PM3_VC_BL_ACTIVE_LOW                          0x00000002
#define              PM3_VC_LD_DISABLE                             0x00000000
#define              PM3_VC_LD_ENABLE                              0x00000004
#define              PM3_VC_HSC_FORCE_HIGH                         0x00000000
#define              PM3_VC_HSC_ACTIVE_HIGH                        0x00000008
#define              PM3_VC_HSC_FORCE_LOW                          0x00000010
#define              PM3_VC_HSC_ACTIVE_LOW                         0x00000018
#define              PM3_VC_VSC_FORCE_HIGH                         0x00000000
#define              PM3_VC_VSC_ACTIVE_HIGH                        0x00000020
#define              PM3_VC_VSC_FORCE_LOW                          0x00000040
#define              PM3_VC_VSC_ACTIVE_LOW                         0x00000060
#define              PM3_VC_PIXELSIZE_8BIT                         0x00000000
#define              PM3_VC_PIXELSIZE_16BIT                        0x00080000
#define              PM3_VC_PIXELSIZE_32BIT                        0x00B00000
#define              PM3_VC_DISPLAY_ENABLE                         0x00010000
#define              PM3_VC_DISPLAY_DISABLE                        0x00000000
#define PM3_DISPLAY_DATA                       0x00003068
#define              PM3_DD_SDA_IN                                 0x00000001
#define              PM3_DD_SCL_IN                                 0x00000002
#define PM3_FIFOCONTROL                        0x00003078
#define PM3_RD_PM3_INDEX_CONTROL               0x00004038
#define              PM3_INCREMENT_ENABLE                          0x00000001
#define              PM3_INCREMENT_DISABLE                         0x00000000
#define PM3_DAC_PAL_WRITE_IDX                  0x00004000
#define PM3_DAC_PAL_DATA                       0x00004008
#define PM3_DAC_INDEX_LOW                      0x00004020
#define PM3_DAC_INDEX_HIGH                     0x00004028
#define PM3_DAC_INDEX_DATA                     0x00004030
#define PM3_DAC_INDEX_CONTROL                  0x00004038

#define PM3_STARTXDOM                          0x00008000
#define PM3_STARTXSUB                          0x00008010
#define PM3_STARTY                             0x00008020
#define PM3_COUNT                              0x00008030
#define PM3_DXDOM                              0x00008008
#define PM3_DXSUB                              0x00008018
#define PM3_DY                                 0x00008028
#define PM3_BITMASKPATTERN                     0x00008068
#define PM3_RASTERIZER_MODE                    0x000080a0
#define              PM3_RM_MASK_MIRROR                            0x00000001 /* mask is right-to-left */
#define PM3_PIXEL_SIZE                         0x000080c0
#define              PM3_PS_8BIT                                   0x00000002
#define              PM3_PS_16BIT                                  0x00000001
#define              PM3_PS_32BIT                                  0x00000000
#define PM3_SPANCOLORMASK                      0x00008168
#define PM3_SCISSOR_MODE                       0x00008180
#define PM3_SCISSORMAXXY                       0x00008190
#define PM3_SCISSORMINXY                       0x00008188
#define PM3_AREASTIPPLE_MODE                   0x000081a0
#define PM3_LINESTIPPLE_MODE                   0x000081a8
#define PM3_TEXTUREADDRESS_MODE                0x00008380
#define PM3_TEXTUREFILTER_MODE                 0x000084e0
#define PM3_TEXTUREREAD_MODE                   0x00008670
#define PM3_TEXTURECOLOR_MODE                  0x00008680
#define PM3_FOG_MODE                           0x00008690
#define PM3_COLORDDA_MODE                      0x000087e0
#define PM3_COLOR	                       0x000087f0
#define PM3_ALPHATEST_MODE                     0x00008800
#define PM3_ANTIALIAS_MODE                     0x00008808
#define PM3_DITHER_MODE                        0x00008818
#define              PM3_CF_TO_DIM_CF(_cf)                         ((((_cf) & 0x0f) << 2) | ( 1 << 10))
#define			PM3_DITHER_ENABLE	0x00000001
#define			PM3_DITHER		0x00000002
#define			PM3_DITHER_COLOR_SHIFT	2
#define			PM3_DITHER_RGB		0x00000400
#define PM3_FBSOFTWAREWRITEMASK                0x00008820
#define PM3_LOGICALOP_MODE                     0x00008828
#define PM3_ROUTER_MODE                        0x00008840
#define PM3_LB_WRITE_MODE                      0x000088c0
#define              PM3_LB_DISABLE                                0x00000000
#define PM3_WINDOW                             0x00008980
#define PM3_STENCIL_MODE                       0x00008988
#define PM3_STENCIL_DATA                       0x00008990
#define PM3_DEPTH_MODE                         0x000089a0
#define PM3_FBWRITE_MODE                       0x00008ab8
#define              PM3_FBWRITEMODE_WRITEENABLE                   0x00000001
#define              PM3_FBWRITEMODE_OPAQUESPAN                    0x00000020
#define              PM3_FBWRITEMODE_ENABLE0                       0x00001000
#define PM3_FBHARDWAREWRITEMASK                0x00008ac0
#define PM3_FILTER_MODE                        0x00008c00
#define              PM3_FM_PASS_SYNC                              0x00000400
#define PM3_STATISTIC_MODE                     0x00008c08
#define PM3_SYNC                               0x00008c40
#define              PM3_SYNC_TAG                                  0x188
#define PM3_YUV_MODE                           0x00008f00
#define PM3_CHROMATEST_MODE                    0x00008f18
#define PM3_DELTA_MODE                         0x00009300
#define PM3_DELTACONTROL                       0x00009350
#define PM3_XBIAS                              0x00009480
#define PM3_YBIAS                              0x00009488
#define PM3_FBDESTREAD_BUFFERADDRESS0          0x0000ae80
#define PM3_FBDESTREAD_BUFFEROFFSET0           0x0000aea0
#define PM3_FBDESTREAD_BUFFERWIDTH0            0x0000aec0
#define              PM3_FBDESTREAD_BUFFERWIDTH_WIDTH(_w)          ((_w) & 0x0fff)
#define PM3_FB_DESTREAD_MODE                   0x0000aee0
#define              PM3_FBDRM_ENABLE                              0x00000001
#define              PM3_FBDRM_ENABLE0                             0x00000100
#define PM3_FBDESTREAD_ENABLE                  0x0000aee8
#define              PM3_FBDESTREAD_SET(_e, _r, _a)                (((_e) & 0xff) | (((_r) & 0xff) << 8) | (((_a) & 0xff) << 24))
#define PM3_FBSOURCEREAD_MODE                  0x0000af00
#define              PM3_FBSOURCEREAD_MODE_ENABLE                  0x00000001
#define              PM3_FBSOURCEREAD_MODE_BLOCKING                0x00000800
#define PM3_FBSOURCEREAD_BUFFERADDRESS         0x0000af08
#define PM3_FBSOURCEREAD_BUFFEROFFSET          0x0000af10
#define PM3_FBSOURCEREAD_BUFFERWIDTH           0x0000af18
#define              PM3_FBSOURCEREAD_BUFFERWIDTH_WIDTH(_w)        ((_w) & 0x0fff)
#define PM3_ALPHA_SOURCE_COLOR		       0x0000af80	/* in ABGR */
#define PM3_ALPHA_DEST_COLOR		       0x0000af88	/* in ABGR */
#define PM3_ALPHABLENDCOLOR_MODE               0x0000afa0
	/* lower 12 bits are identical to PM3_ALPHABLENDALPHA_MODE */
#define			PM3_COLORFORMAT_MASK	0x0000f000
#define			PM3_COLORFORMAT_8888	0x00000000
#define			PM3_COLORFORMAT_4444	0x00001000
#define			PM3_COLORFORMAT_5551	0x00002000
#define			PM3_COLORFORMAT_565	0x00003000
#define			PM3_COLORFORMAT_332	0x00004000
#define			PM3_COLOR_ORDER_RGB	0x00010000
#define			PM3_COLOR_CONV_SHIFT	0x00020000	/* scale otherwise */.
#define			PM3_SOURCE_COLOR_CONST	0x00040000
#define			PM3_DEST_COLOR_CONST	0x00080000
#define			PM3_COLOR_OP_MASK	0x00f00000	/* see PM3_ALPHA_OP_MASK */
#define			PM3_SWAP_SOURCE_DEST	0x01000000
#define PM3_ALPHABLENDALPHA_MODE               0x0000afa8
#define			PM3_ENABLE_ALPHA	0x00000001
#define			PM3_SOURCEBLEND_MASK	0x0000001e
#define			PM3_SOURCEBLEND_SHIFT	1
#define			PM3_DESTBLEND_MASK	0x000000e0
#define			PM3_DESTBLEND_SHIFT	5
#define			PM3_SOURCE_X2		0x00000100
#define			PM3_DEST_X2		0x00000200
#define			PM3_SOURCE_INVERT	0x00000400
#define			PM3_DEST_INVERT		0x00000800
#define			PM3_NO_ALPHA_BUFFER	0x00001000
#define			PM3_ALPHA_APPLE		0x00002000
#define			PM3_ALPHA_OPENGL	0x00000000
#define			PM3_ALPHA_SHIFT		0x00004000
#define			PM3_ALPHA_SCALE		0x00000000
#define			PM3_SOURCE_ALPHA_CONST	0x00008000
#define			PM3_DEST_ALPHA_CONST	0x00010000
#define			PM3_ALPHA_OP_MASK	0x000e0000
#define			PM3_ALPHA_OP_ADD	0x00000000
#define			PM3_ALPHA_OP_SUB	0x00020000
#define			PM3_ALPHA_OP_SUBREV	0x00040000
#define			PM3_ALPHA_OP_MIN	0x00060000
#define			PM3_ALPHA_OP_MAX	0x00080000
#define PM3_FBWRITEBUFFERADDRESS0              0x0000b000
#define PM3_FBWRITEBUFFEROFFSET0               0x0000b020
#define PM3_FBWRITEBUFFERWIDTH0                0x0000b040
#define              PM3_FBWRITEBUFFERWIDTH_WIDTH(_w)              ((_w) & 0x0fff)
#define PM3_SIZEOF_FRAMEBUFFER                 0x0000b0a8
#define PM3_FOREGROUNDCOLOR                    0x0000b0c0
#define PM3_BACKGROUNDCOLOR                    0x0000b0c8
#define PM3_TEXTURECOMPOSITE_MODE              0x0000b300
#define PM3_TEXTURECOMPOSITECOLOR_MODE0        0x0000b308
#define PM3_TEXTURECOMPOSITEALPHA_MODE0        0x0000b310
#define PM3_TEXTURECOMPOSITECOLOR_MODE1        0x0000b318
#define PM3_TEXTURECOMPOSITEALPHA_MODE1        0x0000b320
#define PM3_TEXTUREINDEX_MODE0                 0x0000b338
#define PM3_TEXTUREINDEX_MODE1                 0x0000b340
#define PM3_TEXELLUT_MODE                      0x0000b378
#define PM3_LB_DESTREAD_MODE                   0x0000b500
#define PM3_LB_DESTREAD_ENABLES                0x0000b508
#define PM3_LB_SOURCEREAD_MODE                 0x0000b520
#define PM3_GID_MODE                           0x0000b538
#define PM3_RECTANGLEPOSITION                  0x0000b600
#define PM3_CONFIG2D                           0x0000b618
#define              PM3_CONFIG2D_OPAQUESPAN                       0x00000001
#define              PM3_CONFIG2D_MULTIRXBLIT                      0x00000002
#define              PM3_CONFIG2D_USERSCISSOR_ENABLE               0x00000004
#define              PM3_CONFIG2D_FBDESTREAD_ENABLE                0x00000008
#define              PM3_CONFIG2D_ALPHABLEND_ENABLE                0x00000010
#define              PM3_CONFIG2D_DITHER_ENABLE                    0x00000020
#define              PM3_CONFIG2D_FOREGROUNDROP_ENABLE             0x00000040
#define              PM3_CONFIG2D_FOREGROUNDROP(_rop)               (((_rop) & 0xF) << 7)
#define              PM3_CONFIG2D_BACKGROUNDROP_ENABLE             0x00000800
#define              PM3_CONFIG2D_BACKGROUNDROP(_rop)               (((_rop) & 0xF) << 12)
#define              PM3_CONFIG2D_USECONSTANTSOURCE                0x00010000
#define              PM3_CONFIG2D_FBWRITE_ENABLE                   0x00020000
#define              PM3_CONFIG2D_BLOCKING                         0x00040000
#define              PM3_CONFIG2D_EXTERNALSOURCEDATA               0x00080000
#define              PM3_CONFIG2D_LUTMODE_ENABLE                   0x00100000
#define PM3_RENDER2D                           0x0000b640
#define              PM3_RENDER2D_OPERATION_NORMAL                 0x00000000
#define              PM3_RENDER2D_OPERATION_SYNCONHOSTDATA         0x00001000
#define              PM3_RENDER2D_OPERATION_SYNCONBITMASK          0x00002000
#define              PM3_RENDER2D_OPERATION_PATCHORDERRENDERING    0x00003000
#define              PM3_RENDER2D_FBSOURCEREADENABLE               0x00004000
#define              PM3_RENDER2D_SPANOPERATION                    0x00008000
#define              PM3_RENDER2D_XPOSITIVE                        0x10000000
#define              PM3_RENDER2D_YPOSITIVE                        0x20000000
#define              PM3_RENDER2D_AREASTIPPLEENABLE                0x40000000
#define              PM3_RENDER2D_TEXTUREENABLE                    0x80000000

#define PM3_BLEND_ZERO				0x00000000
#define PM3_BLEND_ONE				0x00000001
#define PM3_BLEND_COLOR				0x00000002
#define PM3_BLEND_ONE_MINUS_COLOR		0x00000003
#define PM3_BLEND_SOURCE_ALPHA			0x00000004
#define PM3_BLEND_ONE_MINUS_SOURCE_ALPHA	0x00000005
#define PM3_BLEND_DEST_ALPHA			0x00000006
#define PM3_BLEND_ONE_MINUS_DEST_ALPHA		0x00000007
#define PM3_BLEND_SAT_SOURCE_ALPHA		0x00000008



#endif /* PM3_REG_H */
