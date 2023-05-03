/*	$NetBSD: zz9kreg.h,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa. Register names derived from original drivers for AmigaOS.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#ifndef ZZ9KREG_H
#define ZZ9KREG_H

/* Expected hardware and firmware versions */
#define ZZ9K_HW_VER			(0x0000)
#define ZZ9K_FW_VER			(0x010D)        /* v1.13 */
#define ZZ9K_FW_VER_MIN			(0x010D)        /* v1.13 */

/* Zorro IDs */
#define ZZ9K_MANID			(0x6D6E)
#define ZZ9K_PRODID_Z2			(0x0003)
#define ZZ9K_PRODID_Z3			(0x0004)

/* Address space */
#define ZZ9K_REG_BASE			(0x00000000)
#define ZZ9K_REG_SIZE			(0x00002000)	/* 8 KB */
#define ZZ9K_FB_BASE			(0x00010000)
#define ZZ9K_FB_SIZE			(0x02FF0000)	/* 48 MB - 64 KB */
#define ZZ9K_RX_BASE			(0x00002000)
#define ZZ9K_RX_SIZE			(0x00002000)	/* 8 KB */
#define ZZ9K_TX_BASE			(0x00008000)
#define ZZ9K_TX_SIZE			(0x00002000)	/* 8 KB */

/* Color mode */
#define ZZ9K_COLOR_8BIT			(0)
#define ZZ9K_COLOR_16BIT		(1)
#define ZZ9K_COLOR_32BIT		(2)
#define ZZ9K_COLOR_15BIT		(3)

/* Modes of ZZ9K_MODE */
#define ZZ9K_MODE_1280x720		(0)
#define ZZ9K_MODE_800x600		(1)
#define ZZ9K_MODE_640x480		(2)
#define ZZ9K_MODE_1024x768		(3)
#define ZZ9K_MODE_1280x1024		(4)
#define ZZ9K_MODE_1920x1080		(5)
#define ZZ9K_MODE_720x576p50		(6)
#define ZZ9K_MODE_1920x1080p50		(7)
#define ZZ9K_MODE_720x480		(8)
#define ZZ9K_MODE_640x512		(9)
#define ZZ9K_MODE_1600x1200		(10)
#define ZZ9K_MODE_2560x1440p30		(11)
#define ZZ9K_MODE_720x576p50_NS_PAL     (12)
#define ZZ9K_MODE_720x480_NS_PAL        (13)
#define ZZ9K_MODE_720x576p50_NS_NTSC    (14)
#define ZZ9K_MODE_720x480_NS_NTSC       (15)
#define ZZ9K_MODE_640x400               (16)
#define ZZ9K_MODE_1920x800              (17)

/* Some registers expect the modes and scale factors shifted */
#define ZZ9K_MODE_COLOR_8BIT		(ZZ9K_COLOR_8BIT  << 8)
#define ZZ9K_MODE_COLOR_16BIT		(ZZ9K_COLOR_16BIT << 8)
#define ZZ9K_MODE_COLOR_32BIT		(ZZ9K_COLOR_32BIT << 8)
#define ZZ9K_MODE_COLOR_15BIT		(ZZ9K_COLOR_15BIT << 8)
#define ZZ9K_MODE_SCALE_0		(0 << 12)
#define ZZ9K_MODE_SCALE_1		(1 << 12)
#define ZZ9K_MODE_SCALE_2		(2 << 12)
#define ZZ9K_MODE_SCALE_3		(3 << 12)

/* Feature of ZZ9K_BLITTER_USER1 for ZZ9K_FEATURE */
#define	ZZ9K_FEATURE_NONE		(0)
#define	ZZ9K_FEATURE_SECONDARY_PALETTE	(1)
#define	ZZ9K_FEATURE_NONSTANDARD_VSYNC	(2)

/* Video capture pan */
#define ZZ9K_CAPTURE_PAN_NTSC		(0x00E00000)
#define ZZ9K_CAPTURE_PAN_PAL		(0x00E00000)
#define ZZ9K_CAPTURE_PAN_VGA		(0x00DFF2F8)

/* Modes of ZZ9K_VIDEO_CAPTURE_MODE */
#define ZZ9K_CAPTURE_OFF		(0)
#define ZZ9K_CAPTURE_ON			(1)

/* Operations of MNTZ_VIDEO_CONTROL_OP */
#define ZZ9K_OP_NOP			(0)
#define ZZ9K_OP_PALETTE			(3)
#define ZZ9K_OP_VSYNC			(5)

#define ZZ9K_PALETTE_SIZE		(256)

/* Options of ZZ9K_BLITTER_OP_COPYRECT */
#define ZZ9K_OPT_REGULAR                (1)
#define ZZ90_OPT_NOMASK                 (2)

/* Video Control */
#define ZZ9K_VIDEO_CTRL_DATA_HI		(0x1000)
#define ZZ9K_VIDEO_CTRL_DATA_LO		(0x1002)
#define ZZ9K_VIDEO_CTRL_OP		(0x1004)
#define ZZ9K_VIDEO_CAPTURE_MODE		(0x1006)
#define ZZ9K_VIDEO_BLANK_STATUS		(0x1600)

/* Bits of ZZ9K_CONFIG */
#define ZZ9K_CONFIG_INT_ETH		(1 << 0)
#define ZZ9K_CONFIG_INT_AUDIO		(1 << 1)
#define ZZ9K_CONFIG_INT_ACK		(1 << 3)
#define ZZ9K_CONFIG_INT_ACK_ETH		(1 << 4)
#define ZZ9K_CONFIG_INT_ACK_AUDIO	(1 << 5)

/* Bits of ZZ9K_AUDIO_CONFIG */
#define ZZ9K_AUDIO_CONFIG_INT_AUDIO	(1 << 0)

/* Parameters of ZZ9K_AUDIO_PARAM */
#define ZZ9K_AP_TX_BUF_OFFS_HI		(0)
#define ZZ9K_AP_TX_BUF_OFFS_LO		(1)
#define ZZ9K_AP_RX_BUF_OFFS_HI		(2)
#define ZZ9K_AP_RX_BUF_OFFS_LO		(3)
#define ZZ9K_AP_DSP_PROG_OFFS_HI	(4)
#define ZZ9K_AP_DSP_PROG_OFFS_LO	(5)
#define ZZ9K_AP_DSP_PARAM_OFFS_HI	(6)
#define ZZ9K_AP_DSP_PARAM_OFFS_LO	(7)
#define ZZ9K_AP_DSP_UPLOAD		(8)
#define ZZ9K_AP_DSP_SET_LOWPASS		(9)
#define ZZ9K_AP_DSP_SET_VOLUMES		(10)
#define ZZ9K_AP_DSP_SET_PREFACTOR	(11)
#define ZZ9K_AP_DSP_SET_EQ_BAND1	(12)
#define ZZ9K_AP_DSP_SET_EQ_BAND2	(13)
#define ZZ9K_AP_DSP_SET_EQ_BAND3	(14)
#define ZZ9K_AP_DSP_SET_EQ_BAND4	(15)
#define ZZ9K_AP_DSP_SET_EQ_BAND5	(16)
#define ZZ9K_AP_DSP_SET_EQ_BAND6	(17)
#define ZZ9K_AP_DSP_SET_EQ_BAND7	(18)
#define ZZ9K_AP_DSP_SET_EQ_BAND8	(29)
#define ZZ9K_AP_DSP_SET_EQ_BAND9	(20)
#define ZZ9K_AP_DSP_SET_EQ_BAND10	(21)
#define ZZ9K_AP_DSP_SET_STEREO_VOLUME	(22)

/* Parameters of ZZ9K_DECODER_PARAM */
#define ZZ9K_DP_DECODE_CLEAR		(0)
#define ZZ9K_DP_DECODE_INIT		(1)
#define ZZ9K_DP_DECODE_RUN		(2)


/* REGISTERS */

/* Config and Video */
#define ZZ9K_HW_VERSION			0x00
#define ZZ9K_MODE			0x02
#define ZZ9K_CONFIG			0x04

#define ZZ9K_SPRITE_X			0x06
#define ZZ9K_SPRITE_Y			0x08

#define ZZ9K_PAN_PTR_HI			0x0A
#define ZZ9K_PAN_PTR_LO			0x0C
#define ZZ9K_VIDEOCAP_VMODE		0x0E

/* Blitter */
#define ZZ9K_BLITTER_X1			0x10
#define ZZ9K_BLITTER_Y1			0x12
#define ZZ9K_BLITTER_X2			0x14
#define ZZ9K_BLITTER_Y2			0x16
#define ZZ9K_BLITTER_ROW_PITCH		0x18
#define ZZ9K_BLITTER_X3			0x1A
#define ZZ9K_BLITTER_Y3			0x1C
#define ZZ9K_BLITTER_RGB_HI		0x1E
#define ZZ9K_BLITTER_RGB_LO		0x20
#define ZZ9K_BLITTER_OP_FILLRECT	0x22
#define ZZ9K_BLITTER_OP_COPYRECT	0x24
#define ZZ9K_BLITTER_OP_FILLTEMPLATE	0x26
#define ZZ9K_BLITTER_SRC_HI		0x28
#define ZZ9K_BLITTER_SRC_LO		0x2A
#define ZZ9K_BLITTER_DST_HI		0x2C
#define ZZ9K_BLITTER_DST_LO		0x2E
#define ZZ9K_BLITTER_COLORMODE		0x30
#define ZZ9K_BLITTER_SRC_PITCH		0x32
#define ZZ9K_BLITTER_RGB2_HI		0x34
#define ZZ9K_BLITTER_RGB2_LO		0x36
#define ZZ9K_BLITTER_OP_P2C		0x38
#define ZZ9K_BLITTER_OP_DRAW_LINE	0x3A
#define ZZ9K_BLITTER_OP_P2D		0x3C
#define ZZ9K_BLITTER_OP_INVERTRECT	0x3E

#define ZZ9K_BLITTER_USER1		0x40
#define ZZ9K_BLITTER_USER2		0x42
#define ZZ9K_BLITTER_USER3		0x44
#define ZZ9K_BLITTER_USER4		0x46

/* Sprite cursor */
#define ZZ9K_SPRITE_BITMAP		0x48
#define ZZ9K_SPRITE_COLORS		0x4A

#define ZZ9K_VBLANK_STATUS		0x4C

/* ? */
#define ZZ9K_SCRATCH_COPY		0x50
#define ZZ9K_CVMODE_PARAM		0x52
#define ZZ9K_CVMODE_VAL			0x54
#define ZZ9K_CVMODE_SEL			0x56
#define ZZ9K_CVMODE			0x58

/* Blitter */
#define ZZ9K_BLITTER_OP_DMA		0x5A
#define ZZ9K_BLITTER_OP_ACC		0x5C
#define ZZ9K_BLITTER_SPLIT_POS		0x5E

/* ? */
#define ZZ9K_SET_FEATURE		0x60

/* Audio */
#define ZZ9K_AUDIO_SWAB			0x70
#define ZZ9K_DECODER_FIFO		0x72
#define ZZ9K_AUDIO_SCALE 		0x74
#define ZZ9K_AUDIO_PARAM		0x76
#define ZZ9K_AUDIO_VAL			0x78
#define ZZ9K_DECODER_PARAM		0x7A
#define ZZ9K_DECODER_VAL		0x7C
#define ZZ9K_DECODE			0x7E

/* Network */
#define ZZ9K_ETH_TX			0x80
#define ZZ9K_ETH_RX			0x82
#define ZZ9K_ETH_MAC_HI			0x84
#define ZZ9K_ETH_MAC_MD			0x86
#define ZZ9K_ETH_MAC_LO			0x88

/* ARM processing */
#define ZZ9K_ARM_RUN_HI			0x90
#define ZZ9K_ARM_RUN_LO			0x92
#define ZZ9K_ARM_ARGC			0x94
#define ZZ9K_ARM_ARGV0			0x96
#define ZZ9K_ARM_ARGV1			0x98
#define ZZ9K_ARM_ARGV2			0x9A
#define ZZ9K_ARM_ARGV3			0x9C
#define ZZ9K_ARM_ARGV4			0x9E
#define ZZ9K_ARM_ARGV5			0xA0
#define ZZ9K_ARM_ARGV6			0xA2
#define ZZ9K_ARM_ARGV7			0xA4

#define ZZ9K_ARM_EVENT_SERIAL		0xB0
#define ZZ9K_ARM_EVENT_CODE		0xB2

/* Board hardware */
#define ZZ9K_FW_VERSION			0xC0

/* USB */
#define ZZ9K_USB_TX_HI			0xD0
#define ZZ9K_USB_TX_LO			0xD2
#define ZZ9K_USB_RX_HI			0xD4
#define ZZ9K_USB_RX_LO			0xD6
#define ZZ9K_USB_STATUS			0xD8
#define ZZ9K_USB_BUFSEL			0xDA
#define ZZ9K_USB_CAPACITY		0xDC

/* Hardware Status */
#define ZZ9K_TEMPERATURE		0xE0
#define ZZ9K_VOLTAGE_AUX		0xE2
#define ZZ9K_VOLTAGE_CORE		0xE4

/* Misccellaneous */
#define ZZ9K_PRINT_CHR			0xF0
#define ZZ9K_PRINT_HEX			0xF2
#define ZZ9K_AUDIO_CONFIG		0xF4

#define ZZ9K_DEBUG			0xFC
#define ZZ9K_DEBUG_TIMER		0xFE


#endif /* ZZ9KREG_H */
