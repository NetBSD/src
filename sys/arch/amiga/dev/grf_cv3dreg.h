/*	$NetBSD: grf_cv3dreg.h,v 1.7.26.1 2007/03/12 05:46:40 rmind Exp $	*/

/*
 * Copyright (c) 1995 Michael Teske
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
 *      This product includes software developed by Ezra Story and  by Kari
 *      Mettinen.
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

#ifndef _GRF_CV3DREG_H
#define _GRF_CV3DREG_H

/*
 * This is derived from ciruss driver source
 */

/* Extension to grfvideo_mode to support text modes.
 * This can be passed to both text & gfx functions
 * without worry.  If gv.depth == 4, then the extended
 * fields for a text mode are present.
 */

struct grfcv3dtext_mode {
	struct grfvideo_mode gv;
	unsigned short	fx;	/* font x dimension */
	unsigned short	fy;	/* font y dimension */
	unsigned short	cols;	/* screen dimensions */
	unsigned short	rows;
	void		*fdata;	/* font data */
	unsigned short	fdstart;
	unsigned short	fdend;
};

/* maximum console size */
#define MAXROWS 200
#define MAXCOLS 200

/* read VGA register */
#define vgar(ba, reg) \
	*(((volatile char *)ba)+(reg ^ 3))

/* write VGA register */
#define vgaw(ba, reg, val) \
	*(((volatile char *)ba)+(reg ^ 3)) = ((val) & 0xff)

/* MMIO access */
#define ByteAccessIO(x)	( ((x) & 0x3ffc) | (((x) & 3)^3) | (((x) & 3) <<14) )

#define vgario(ba, reg) \
	*(((volatile char *)ba) + ( ByteAccessIO(reg) ))

#define vgawio(ba, reg, val) \
	do { \
		if (!cv3d_zorroIII) { \
		        *(((volatile char *)cv3d_vcode_switch_base) + \
			    0x04) = (0x01 & 0xffff); \
			__asm volatile ("nop"); \
		} \
		*(((volatile char *)cv3d_special_register_base) + \
		    ( ByteAccessIO(reg) & 0xffff )) = ((val) & 0xff); \
		if (!cv3d_zorroIII) { \
		        *(((volatile char *)cv3d_vcode_switch_base) + \
			    0x04) = (0x02 & 0xffff); \
			__asm volatile ("nop"); \
		} \
	} while (0)

/* read 32 Bit VGA register */
#define vgar32(ba, reg) \
	*((volatile unsigned long *) (((volatile char *)ba)+reg))

/* write 32 Bit VGA register */
#define vgaw32(ba, reg, val) \
	*((volatile unsigned long *) (((volatile char *)ba)+reg)) = val

/* read 16 Bit VGA register */
#define vgar16(ba, reg) \
	*((volatile unsigned short *) (((volatile char *)ba)+reg))

/* write 16 Bit VGA register */
#define vgaw16(ba, reg, val) \
	*((volatile unsigned short *) (((volatile char *)ba)+reg)) = val

/* XXX This is totaly untested */
#define	Select_Zorro2_FrameBuffer(flag) \
	do { \
		*(((volatile char *)cv3d_vcode_switch_base) + \
		    0x08) = ((flag * 0x40) & 0xffff); \
		__asm volatile ("nop"); \
} while (0)

int grfcv3d_cnprobe(void);
void grfcv3d_iteinit(struct grf_softc *);
static inline void GfxBusyWait(volatile void *);
static inline void GfxFifoWait(volatile void *);
static inline unsigned char RAttr(volatile void *, short);
static inline unsigned char RSeq(volatile void *, short);
static inline unsigned char RCrt(volatile void *, short);
static inline unsigned char RGfx(volatile void *, short);


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
#define GREG_MISC_OUTPUT_R	0x03CC
#define GREG_MISC_OUTPUT_W	0x03C2
#define GREG_FEATURE_CONTROL_R	0x03CA
#define GREG_FEATURE_CONTROL_W	0x03DA
#define GREG_INPUT_STATUS0_R	0x03C2
#define GREG_INPUT_STATUS1_R	0x03DA

/* Setup Registers: */
#define SREG_OPTION_SELECT	0x0102
#define SREG_VIDEO_SUBS_ENABLE	0x03C3	/* Trio64: 0x46E8 */

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
#define ACT_ID_COLOR_SELECT	0x14	/* ACT_ID_PIXEL_PADDING */

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
#define SEQ_ID_UNKNOWN1		0x05
#define SEQ_ID_UNKNOWN2		0x06
#define SEQ_ID_UNKNOWN3		0x07
/* S3 extensions */
#define SEQ_ID_UNLOCK_EXT	0x08
#define SEQ_ID_MMIO_SELECT	0x09	/* Trio64: SEQ_ID_EXT_SEQ_REG9 */
#define SEQ_ID_BUS_REQ_CNTL	0x0A
#define SEQ_ID_EXT_MISC_SEQ	0x0B
#define SEQ_ID_UNKNOWN4		0x0C
#define SEQ_ID_EXT_SEQ		0x0D
#define SEQ_ID_UNKNOWN5		0x0E
#define SEQ_ID_UNKNOWN6		0x0F
#define SEQ_ID_MCLK_LO		0x10
#define SEQ_ID_MCLK_HI		0x11
#define SEQ_ID_DCLK_LO		0x12
#define SEQ_ID_DCLK_HI		0x13
#define SEQ_ID_CLKSYN_CNTL_1	0x14
#define SEQ_ID_CLKSYN_CNTL_2	0x15
#define SEQ_ID_CLKSYN_TEST_HI	0x16	/* reserved for S3 testing of the */
#define SEQ_ID_CLKSYN_TEST_LO	0x17	/*   internal clock synthesizer   */
#define SEQ_ID_RAMDAC_CNTL	0x18
#define SEQ_ID_MORE_MAGIC	0x1A	/* not available on the Virge */
#define SEQ_ID_SIGNAL_SELECT	0x1C

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
#define CRT_ID_MAX_SCAN_LINE	0x09
#define CRT_ID_CURSOR_START	0x0A
#define CRT_ID_CURSOR_END	0x0B
#define CRT_ID_START_ADDR_HIGH	0x0C
#define CRT_ID_START_ADDR_LOW	0x0D
#define CRT_ID_CURSOR_LOC_HIGH	0x0E
#define CRT_ID_CURSOR_LOC_LOW	0x0F
#define CRT_ID_START_VER_RETR	0x10
#define CRT_ID_END_VER_RETR	0x11
#define CRT_ID_VER_DISP_ENA_END	0x12
#define CRT_ID_SCREEN_OFFSET	0x13
#define CRT_ID_UNDERLINE_LOC	0x14
#define CRT_ID_START_VER_BLANK	0x15
#define CRT_ID_END_VER_BLANK	0x16
#define CRT_ID_MODE_CONTROL	0x17
#define CRT_ID_LINE_COMPARE	0x18
#define CRT_ID_GD_LATCH_RBACK	0x22
#define CRT_ID_ACT_TOGGLE_RBACK	0x24
#define CRT_ID_ACT_INDEX_RBACK	0x26
/* S3 extensions: S3 VGA Registers */
#define CRT_ID_DEVICE_HIGH	0x2D
#define CRT_ID_DEVICE_LOW	0x2E
#define CRT_ID_REVISION 	0x2F
#define CRT_ID_CHIP_ID_REV	0x30
#define CRT_ID_MEMORY_CONF	0x31
#define CRT_ID_BACKWAD_COMP_1	0x32
#define CRT_ID_BACKWAD_COMP_2	0x33
#define CRT_ID_BACKWAD_COMP_3	0x34
#define CRT_ID_REGISTER_LOCK	0x35
#define CRT_ID_CONFIG_1 	0x36
#define CRT_ID_CONFIG_2 	0x37
#define CRT_ID_REGISTER_LOCK_1	0x38
#define CRT_ID_REGISTER_LOCK_2	0x39
#define CRT_ID_MISC_1		0x3A
#define CRT_ID_DISPLAY_FIFO	0x3B
#define CRT_ID_LACE_RETR_START	0x3C
/* S3 extensions: System Control Registers  */
#define CRT_ID_SYSTEM_CONFIG	0x40
#define CRT_ID_BIOS_FLAG	0x41
#define CRT_ID_LACE_CONTROL	0x42
#define CRT_ID_EXT_MODE 	0x43
#define CRT_ID_HWGC_MODE	0x45	/* HWGC = Hardware Graphics Cursor */
#define CRT_ID_HWGC_ORIGIN_X_HI	0x46
#define CRT_ID_HWGC_ORIGIN_X_LO	0x47
#define CRT_ID_HWGC_ORIGIN_Y_HI	0x48
#define CRT_ID_HWGC_ORIGIN_Y_LO	0x49
#define CRT_ID_HWGC_FG_STACK	0x4A
#define CRT_ID_HWGC_BG_STACK	0x4B
#define CRT_ID_HWGC_START_AD_HI	0x4C
#define CRT_ID_HWGC_START_AD_LO	0x4D
#define CRT_ID_HWGC_DSTART_X	0x4E
#define CRT_ID_HWGC_DSTART_Y	0x4F
/* S3 extensions: System Extension Registers  */
#define CRT_ID_EXT_SYS_CNTL_1	0x50
#define CRT_ID_EXT_SYS_CNTL_2	0x51
#define CRT_ID_EXT_BIOS_FLAG_1	0x52
#define CRT_ID_EXT_MEM_CNTL_1	0x53
#define CRT_ID_EXT_MEM_CNTL_2	0x54
#define CRT_ID_EXT_DAC_CNTL	0x55
#define CRT_ID_EX_SYNC_1	0x56
#define CRT_ID_EX_SYNC_2	0x57
#define CRT_ID_LAW_CNTL		0x58	/* LAW = Linear Address Window */
#define CRT_ID_LAW_POS_HI	0x59
#define CRT_ID_LAW_POS_LO	0x5A
#define CRT_ID_GOUT_PORT	0x5C
#define CRT_ID_EXT_HOR_OVF	0x5D
#define CRT_ID_EXT_VER_OVF	0x5E
#define CRT_ID_EXT_MEM_CNTL_3	0x60
#define CRT_ID_EXT_MEM_CNTL_4	0x61	/* only available on the Virge */
#define CRT_ID_EX_SYNC_3	0x63	/* not available on the Virge */
#define CRT_ID_EXT_MISC_CNTL	0x65
#define CRT_ID_EXT_MISC_CNTL_1	0x66
#define CRT_ID_EXT_MISC_CNTL_2	0x67
#define CRT_ID_CONFIG_3 	0x68
#define CRT_ID_EXT_SYS_CNTL_3	0x69
#define CRT_ID_EXT_SYS_CNTL_4	0x6A
#define CRT_ID_EXT_BIOS_FLAG_3	0x6B
#define CRT_ID_EXT_BIOS_FLAG_4	0x6C
#define CRT_ID_EXT_BIOS_FLAG_5	0x6D	/* only available on the Virge */
#define CRT_ID_RAMDAC_SIG_TEST	0x6E	/* only available on the Virge */
#define CRT_ID_CONFIG_4 	0x6F	/* only available on the Virge */

/* Streams Processor */
#define SP_PRIMARY_CONTROL		0x8180
#define SP_COLOR_CHROMA_KEY_CONTROL	0x8184
#define SP_SECONDARY_CONTROL		0x8190
#define SP_CHROMA_KEY_UPPER_BOUND	0x8194
#define SP_SECONDARY_CONSTANTS		0x8198
#define SP_BLEND_CONTROL		0x81A0
#define SP_PRIMARY_ADDRESS_0		0x81C0
#define SP_PRIMARY_ADDRESS_1		0x81C4
#define SP_PRIMARY_STRIDE		0x81C8
#define SP_DOUBLE_BUFFER_LPB_SUPPORT	0x81CC
#define SP_SECONDARY_ADDRESS_0		0x81D0
#define SP_SECONDARY_ADDRESS_1		0x81D4
#define SP_SECONDARY_STRIDE		0x81D8
#define SP_OPAQUE_OVERLAY_CONTROL	0x81DC
#define SP_K1_VERTICAL_SCALE_FACTOR	0x81E0
#define SP_K2_VERTICAL_SCALE_FACTOR	0x81E4
#define SP_DDA_VERTICAL_ACCUMULATOR	0x81E8
#define SP_FIFO_CONTROL			0x81EC
#define SP_PRIMARY_WINDOW_TOP_LEFT	0x81F0
#define SP_PRIMARY_WINDOW_SIZE		0x81F4
#define SP_SECONDARY_WINDOW_TOP_LEFT	0x81F8
#define SP_SECONDARY_WINDOW_SIZE	0x81FC

/* Memory Port Controller */
#define MPC_FIFO_CONTROL		0x8200
#define MPC_MIU_CONTROL			0x8204
#define MPC_STREAMS_TIMEOUT		0x8208
#define MPC_MISC_TIMEOUT		0x820C
#define MPC_DMA_READ_BASE_ADDRESS	0x8220
#define MPC_DMA_READ_STRIDE_WIDTH	0x8224

/* Miscellaneous Registers */
#define MR_SUBSYSTEM_STATUS_CNTL	0x8504
#define MR_ADVANCED_FUNCTION_CONTROL	0x850C

/* S3d Engine */
#define S3D_BIT_BLT_RECT_FILL		0xA400
#define S3D_LINE_2D			0xA800
#define S3D_POLYGON_2D			0xAC00
#define S3D_LINE_3D			0xB000
#define S3D_TRIANGLE_3D			0xB400

#define BLT_ADDRESS			0xA4D4
#define BLT_SOURCE_ADDRESS		0xA4D4
#define BLT_DEST_ADDRESS		0xA4D8
#define BLT_CLIP_LEFT_RIGHT		0xA4DC
#define BLT_CLIP_LEFT			BLT_CLIP_LEFT_RIGHT
#define BLT_CLIP_RIGHT			0xA4DE
#define BLT_CLIP_TOP_BOTTOM		0xA4E0
#define BLT_CLIP_BOTTOM			BLT_CLIP_TOP_BOTTOM
#define BLT_CLIP_TOP			0xA4E2
#define BLT_DEST_SOURCE_PITCH		0xA4E4
#define BLT_SOURCE_PITCH		BLT_DEST_SOURCE_PITCH
#define BLT_DEST_PITCH			0xA4E6
#define BLT_MONO_PATTERN		0xA4E8
#define BLT_MONO_PATTERN_0		BLT_MONO_PATTERN
#define BLT_MONO_PATTERN_1		0xA4EC
#define BLT_PATTERN_BG_COLOR		0xA4F0
#define BLT_PATTERN_BG_COLOR_TRUE_COLOR	BLT_PATTERN_BG_COLOR
#define BLT_PATTERN_BG_COLOR_ALPHA	BLT_PATTERN_BG_COLOR
#define BLT_PATTERN_BG_COLOR_RED	0xA4F1
#define BLT_PATTERN_BG_COLOR_HI_COLOR	0xA4F2
#define BLT_PATTERN_BG_COLOR_GREEN	BLT_PATTERN_BG_COLOR_HI_COLOR
#define BLT_PATTERN_BG_COLOR_INDEX	0xA4F3
#define BLT_PATTERN_BG_COLOR_BLUE	BLT_PATTERN_BG_COLOR_INDEX
#define BLT_PATTERN_FG_COLOR		0xA4F4
#define BLT_PATTERN_FG_COLOR_TRUE_COLOR	BLT_PATTERN_FG_COLOR
#define BLT_PATTERN_FG_COLOR_ALPHA	BLT_PATTERN_FG_COLOR
#define BLT_PATTERN_FG_COLOR_RED	0xA4F5
#define BLT_PATTERN_FG_COLOR_HI_COLOR	0xA4F6
#define BLT_PATTERN_FG_COLOR_GREEN	BLT_PATTERN_FG_COLOR_HI_COLOR
#define BLT_PATTERN_FG_COLOR_INDEX	0xA4F7
#define BLT_PATTERN_FG_COLOR_BLUE	BLT_PATTERN_FG_COLOR_INDEX
#define BLT_SOURCE_BG_COLOR		0xA4F8
#define BLT_SOURCE_BG_COLOR_TRUE_COLOR	BLT_SOURCE_BG_COLOR
#define BLT_SOURCE_BG_COLOR_ALPHA	BLT_SOURCE_BG_COLOR
#define BLT_SOURCE_BG_COLOR_RED		0xA4F9
#define BLT_SOURCE_BG_COLOR_HI_COLOR	0xA4FA
#define BLT_SOURCE_BG_COLOR_GREEN	BLT_SOURCE_BG_COLOR_HI_COLOR
#define BLT_SOURCE_BG_COLOR_INDEX	0xA4FB
#define BLT_SOURCE_BG_COLOR_BLUE	BLT_SOURCE_BG_COLOR_INDEX
#define BLT_SOURCE_FG_COLOR		0xA4FC
#define BLT_SOURCE_FG_COLOR_TRUE_COLOR	BLT_SOURCE_FG_COLOR
#define BLT_SOURCE_FG_COLOR_ALPHA	BLT_SOURCE_FG_COLOR
#define BLT_SOURCE_FG_COLOR_RED		0xA4FD
#define BLT_SOURCE_FG_COLOR_HI_COLOR	0xA4FE
#define BLT_SOURCE_FG_COLOR_GREEN	BLT_SOURCE_FG_COLOR_HI_COLOR
#define BLT_SOURCE_FG_COLOR_INDEX	0xA4FF
#define BLT_SOURCE_FG_COLOR_BLUE	BLT_SOURCE_FG_COLOR_INDEX
#define BLT_COMMAND_SET			0xA500
#define BLT_WIDTH_HEIGHT		0xA504
#define BLT_HEIGHT			BLT_WIDTH_HEIGHT
#define BLT_WIDTH 			0xA506
#define BLT_SOURCE_XY			0xA508
#define BLT_SOURCE_Y			BLT_SOURCE_XY
#define BLT_SOURCE_X			0xA50A
#define BLT_DESTINATION_XY		0xA50C
#define BLT_DESTINATION_Y 		BLT_DESTINATION_XY
#define BLT_DESTINATION_X		0xA50E

#define L2D_ADDRESS			0xA8D4
#define L2D_SOURCE_ADDRESS		0xA8D4
#define L2D_DEST_ADDRESS		0xA8D8
#define L2D_CLIP_LEFT_RIGHT		0xA8DC
#define L2D_CLIP_LEFT			L2D_CLIP_LEFT_RIGHT
#define L2D_CLIP_RIGHT			0xA8DE
#define L2D_CLIP_TOP_BOTTOM		0xA8E0
#define L2D_CLIP_BOTTOM			L2D_CLIP_TOP_BOTTOM
#define L2D_CLIP_TOP			0xA8E2
#define L2D_DEST_SOURCE_PITCH		0xA8E4
#define L2D_SOURCE_PITCH		L2D_DEST_SOURCE_PITCH
#define L2D_DEST_PITCH			0xA8E6
#define L2D_PAD_0			0xA8E8
#define L2D_PATTERN_FG_COLOR_TRUE_COLOR	0xA8F4
#define L2D_PATTERN_FG_COLOR_ALPHA	L2D_PATTERN_FG_COLOR_TRUECOLOR
#define L2D_PATTERN_FG_COLOR_RED	0xA8F5
#define L2D_PATTERN_FG_COLOR_HI_COLOR	0xA8F6
#define L2D_PATTERN_FG_COLOR_GREEN	L2D_PATTERN_FG_COLOR_HICOLOR
#define L2D_PATTERN_FG_COLOR_INDEX	0xA8F7
#define L2D_PATTERN_FG_COLOR_BLUE	L2D_PATTERN_FG_COLOR_INDEX
#define L2D_PAD_1			0xA8F8
#define L2D_COMMAND_SET			0xA900
#define L2D_PAD_2			0xA904
#define L2D_END_0_END_1			0xA96C
#define L2D_END_1			L2D_END_0_END_1
#define L2D_END_0			0xA96E
#define L2D_DX				0xA970
#define L2D_X_START			0xA974
#define L2D_Y_START			0xA978
#define L2D_Y_COUNT			0xA97C

#define P2D_ADDRESS			0xACD4
#define P2D_SOURCE_ADDRESS		0xACD4
#define P2D_DEST_ADDRESS		0xACD8
#define P2D_CLIP_LEFT_RIGHT		0xACDC
#define P2D_CLIP_LEFT			P2D_CLIP_LEFT_RIGHT
#define P2D_CLIP_RIGHT			0xACDE
#define P2D_CLIP_TOP_BOTTOM		0xACE0
#define P2D_CLIP_BOTTOM			P2D_CLIP_TOP_BOTTOM
#define P2D_CLIP_TOP			0xACE2
#define P2D_DEST_SOURCE_PITCH		0xACE4
#define P2D_SOURCE_PITCH		P2D_DEST_SOURCE_PITCH
#define P2D_DEST_PITCH			0xACE6
#define P2D_MONO_PATTERN		0xACE8
#define P2D_PATTERN_BG_COLOR_TRUE_COLOR	0xACF0
#define P2D_PATTERN_BG_COLOR_ALPHA	P2D_PATTERN_BG_COLOR_TRUE_COLOR
#define P2D_PATTERN_BG_COLOR_RED	0xACF1
#define P2D_PATTERN_BG_COLOR_HI_COLOR	0xACF2
#define P2D_PATTERN_BG_COLOR_GREEN	P2D_PATTERN_BG_COLOR_HI_COLOR
#define P2D_PATTERN_BG_COLOR_INDEX	0xACF3
#define P2D_PATTERN_BG_COLOR_BLUE	P2D_PATTERN_BG_COLOR_INDEX
#define P2D_PATTERN_FG_COLOR_TRUE_COLOR	0xACF4
#define P2D_PATTERN_FG_COLOR_ALPHA	P2D_PATTERN_FG_COLOR_TRUE_COLOR
#define P2D_PATTERN_FG_COLOR_RED	0xACF5
#define P2D_PATTERN_FG_COLOR_HI_COLOR	0xACF6
#define P2D_PATTERN_FG_COLOR_GREEN	P2D_PATTERN_FG_COLOR_HI_COLOR
#define P2D_PATTERN_FG_COLOR_INDEX	0xACF7
#define P2D_PATTERN_FG_COLOR_BLUE	P2D_PATTERN_FG_COLOR_INDEX
#define P2D_PAD_1			0xACF8
#define P2D_COMMAND_SET			0xAD00
#define P2D_PAD_2			0xAD04
#define P2D_RIGHT_DX			0xAD68
#define P2D_RIGHT_X_START		0xAD6C
#define P2D_LEFT_DX			0xAD70
#define P2D_LEFT_X_START		0xAD74
#define P2D_Y_START			0xAD78
#define P2D_Y_COUNT			0xAD7C

#define CMD_NOP			(7 << 27)	/* %1111 << 27 */
#define CMD_LINE		(3 << 27)	/* %0011 << 27 */
#define CMD_RECT		(4 << 27)	/* %0010 << 27 */
#define CMD_POLYGON		(5 << 27)	/* %0101 << 27 */
#define CMD_BITBLT		(0 << 27)	/* %0000 << 27 */

#define CMD_SKIP_TRANSFER_BYTES_1	(1 << 12)	/* %01 << 12 */
#define CMD_SKIP_TRANSFER_BYTES_2	(2 << 12)	/* %10 << 12 */
#define CMD_SKIP_TRANSFER_BYTES_3	(3 << 12)	/* %11 << 12 */

#define CMD_TRANSFER_ALIGNMENT_BYTE	(0 << 10)	/* %00 << 10 */
#define CMD_TRANSFER_ALIGNMENT_WORD	(1 << 10)	/* %01 << 10 */
#define CMD_TRANSFER_ALIGNMENT_DOUBLEWORD	(2 << 10)	/* %10 << 10 */

#define CMD_CHUNKY	(0 << 2)	/* %00 << 2 */
#define CMD_HI_COLOR	(1 << 2)	/* %01 << 2 */
#define CMD_TRUE_COLOR	(2 << 2)	/* %10 << 2 */

#define ROP_FALSE	0x00
#define ROP_NOR		0x10
#define ROP_ONLYDST	0x20
#define ROP_NOTSRC	0x30
#define ROP_ONLYSRC	0x40
#define ROP_NOTDST	0x50
#define ROP_EOR		0x60
#define ROP_NAND	0x70
#define ROP_AND		0x80
#define ROP_NEOR	0x90
#define ROP_DST		0xA0
#define ROP_NOTONLYSRC	0xB0
#define ROP_SRC		0xC0
#define ROP_NOTONLYDST	0xD0
#define ROP_OR		0xE0
#define ROP_TRUE	0xF0

/* Pass-through */
#if 0	/* XXX */
#define PASS_ADDRESS		0x
#define PASS_ADDRESS_W		0x
#endif

/* Video DAC */
#define VDAC_ADDRESS		0x03C8
#define VDAC_ADDRESS_W		0x03C8
#define VDAC_ADDRESS_R		0x03C7
#define VDAC_STATE		0x03C7
#define VDAC_DATA		0x03C9
#define VDAC_MASK		0x03C6


#define WGfx(ba, idx, val) \
	do { vgaw(ba, GCT_ADDRESS, idx); vgaw(ba, GCT_ADDRESS_W , val); } while (0)

#define WSeq(ba, idx, val) \
	do { vgaw(ba, SEQ_ADDRESS, idx); vgaw(ba, SEQ_ADDRESS_W , val); } while (0)

#define WCrt(ba, idx, val) \
	do { vgaw(ba, CRT_ADDRESS, idx); vgaw(ba, CRT_ADDRESS_W , val); } while (0)

#define WAttr(ba, idx, val) \
	do {	\
		unsigned char tmp;\
		tmp = vgar(ba, ACT_ADDRESS_RESET);\
		vgaw(ba, ACT_ADDRESS_W, idx);\
		vgaw(ba, ACT_ADDRESS_W, val);\
	} while (0)


#define SetTextPlane(ba, m) \
	do { \
		WGfx(ba, GCT_ID_READ_MAP_SELECT, m & 3 );\
		WSeq(ba, SEQ_ID_MAP_MASK, (1 << (m & 3)));\
	} while (0)


/* Gfx engine busy wait */

static inline void
GfxBusyWait (ba)
	volatile void *ba;
{
	int test;

	do {
		test = vgar32(ba, MR_SUBSYSTEM_STATUS_CNTL);
		__asm volatile ("nop");
	} while (!(test & (1 << 13)));
}


static inline void
GfxFifoWait(ba)
	volatile void *ba;
{
#if 0	/* XXX */
	int test;

	do {
		test = vgar32(ba, MR_SUBSYSTEM_STATUS_CNTL);
	} while (test & 0x0f);
#endif
}


/* Special wakeup/passthrough registers on graphics boards
 *
 * The methods have diverged a bit for each board, so
 * WPass(P) has been converted into a set of specific
 * inline functions.
 */

static inline unsigned char
RAttr(ba, idx)
	volatile void *ba;
	short idx;
{

	vgaw(ba, ACT_ADDRESS_W, idx);
	delay(0);
	return vgar(ba, ACT_ADDRESS_R);
}

static inline unsigned char
RSeq(ba, idx)
	volatile void *ba;
	short idx;
{
	vgaw(ba, SEQ_ADDRESS, idx);
	return vgar(ba, SEQ_ADDRESS_R);
}

static inline unsigned char
RCrt(ba, idx)
	volatile void *ba;
	short idx;
{
	vgaw(ba, CRT_ADDRESS, idx);
	return vgar(ba, CRT_ADDRESS_R);
}

static inline unsigned char
RGfx(ba, idx)
	volatile void *ba;
	short idx;
{
	vgaw(ba, GCT_ADDRESS, idx);
	return vgar(ba, GCT_ADDRESS_R);
}

#endif /* _GRF_CV3DREG_H */
