/*	$NetBSD: mgafbreg.h,v 1.1 2026/03/17 10:03:02 macallan Exp $	*/

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef MGAFBREG_H
#define MGAFBREG_H

#define MILL_BAR_REG PCI_MAPREG_START
#define MILL_BAR_FB (PCI_MAPREG_START + 4)
#define MILL2_BAR_REG (PCI_MAPREG_START + 4)
#define MILL2_BAR_FB PCI_MAPREG_START

#define MGA_DWGCTL	0x1C00	/* drawing control */
#define MGA_MACCESS	0x1C04	/* memory access / pixel width */
#define MGA_ZORG	0x1C0C	/* Z-buffer origin */
#define MGA_PLNWT	0x1C1C	/* plane write mask */
#define MGA_BCOL	0x1C20	/* background colour */
#define MGA_FCOL	0x1C24	/* foreground colour */
#define MGA_SHIFT	0x1C50	/* BLT/ILOAD bit-shift within first dword */
#define MGA_SGN		0x1C58	/* scan direction for BLT */
#define MGA_AR0		0x1C60	/* BLT/ILOAD: last pixel in mono stream (triggers start) */
#define MGA_AR3		0x1C6C	/* BLT/ILOAD: first pixel in mono stream */
#define MGA_AR5		0x1C74	/* BLT source pitch (±displayWidth) */
#define MGA_CXBNDRY	0x1C80	/* X clipping: [31:16]=cxright (excl), [15:0]=cxleft (incl) */
#define MGA_FXBNDRY	0x1C84	/* BLT destination X bounds [right:left] */
#define MGA_YDSTLEN	0x1C88	/* BLT destination Y start [31:16] + height [15:0] */
#define MGA_PITCH	0x1C8C	/* pitch (in pixels) */
#define MGA_YDST	0x1C90	/* Y destination */
#define MGA_YDSTORG	0x1C94	/* Y destination origin (framebuffer base) */
#define MGA_YTOP	0x1C98	/* Y top clipping limit (linearized line address) */
#define MGA_YBOT	0x1C9C	/* Y bottom clipping limit (linearized line address) */

#define MGA_EXEC	0x0100	/* OR with any 0x1Cxx register to auto-execute */

/* MGA_MACCESS pixel width field (bits 1:0) and WRAM init trigger. */
#define MGA_MACCESS_WRAM_INIT	0x00008000	/* trigger WRAM init cycle */

#define MGA_DWGCTL_BITBLT	0x00000008UL	/* opcode: screen-to-screen */
#define MGA_DWGCTL_RPL		0x00000000UL	/* atype: replace */
#define MGA_DWGCTL_BOP_COPY	0x000C0000UL	/* bop: GXcopy */
#define MGA_DWGCTL_SHIFTZERO	0x00004000UL	/* shfzero: no pixel shift */
#define MGA_DWGCTL_BFCOL	0x04000000UL	/* bfcol: source from WRAM */

/* Combined DWGCTL value for a plain screen-to-screen copy. */
#define MGA_DWGCTL_COPY \
    (MGA_DWGCTL_BITBLT | MGA_DWGCTL_RPL | MGA_DWGCTL_BOP_COPY | \
     MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_BFCOL)

#define MGA_DWGCTL_TRAP		0x00000004UL	/* opcode: trap/fill */
#define MGA_DWGCTL_SOLID	0x00000800UL	/* solid: fill from FCOL */
#define MGA_DWGCTL_ARZERO	0x00001000UL	/* arzero: AR regs = 0 */
#define MGA_DWGCTL_SGNZERO	0x00002000UL	/* sgnzero: SGN = 0 */
#define MGA_DWGCTL_BMONOLEF	0x00080000UL	/* bmonolef: left-edge first */

/* Combined DWGCTL value for a solid rectangle fill (GXcopy). */
#define MGA_DWGCTL_FILL \
    (MGA_DWGCTL_TRAP | MGA_DWGCTL_SOLID | MGA_DWGCTL_ARZERO | \
     MGA_DWGCTL_SGNZERO | MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_BMONOLEF | \
     MGA_DWGCTL_BOP_COPY)

#define MGA_DWGCTL_ILOAD	0x00000009UL	/* opcod: ILOAD (host-to-screen) */
#define MGA_DWGCTL_LINEAR	0x00000080UL	/* linear: stream source (bit 7) */
#define MGA_DWGCTL_BMONOWF	0x08000000UL	/* bltmod: monochrome word format */
#define MGA_DWGCTL_TRANSC	0x40000000UL	/* transc: transparent background */

#define MGA_DWGCTL_ILOAD_OPAQUE \
    (MGA_DWGCTL_ILOAD | MGA_DWGCTL_RPL | MGA_DWGCTL_BOP_COPY | \
     MGA_DWGCTL_SGNZERO | MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_BMONOWF)

/*
 * ILOAD in full-color mode (BFCOL): host data is native-format pixels,
 * not monochrome bits.  For 16bpp, each 32-bit DMA write = 2 pixels.
 * Used for antialiased font rendering (CPU blends, hardware streams).
 */
#define MGA_DWGCTL_ILOAD_COLOR \
    (MGA_DWGCTL_ILOAD | MGA_DWGCTL_RPL | MGA_DWGCTL_BOP_COPY | \
     MGA_DWGCTL_SGNZERO | MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_BFCOL)

#define MGA_DMAWIN	0x0000	/* ILOAD/IDUMP host data window */

#define MGA_SGN_BLIT_LEFT	0x00000001UL	/* scan right-to-left */
#define MGA_SGN_BLIT_UP		0x00000004UL	/* scan bottom-to-top */

#define MGA_FIFOSTATUS	0x1E10	/* FIFO free slot count (RO) */
#define MGA_STATUS	0x1E14	/* chip status (RO) */
#define MGA_RST		0x1E40	/* hardware reset (write 1=assert, 0=deassert) */
#define MGA_OPMODE	0x1E54	/* operation mode (R/W) */

#define MGA_DWGENGSTS	0x00010000	/* drawing engine busy */

#define MGA_VGA_MISC_W		0x1FC2	/* Miscellaneous output (W) */
#define MGA_VGA_MISC_R		0x1FCC	/* Miscellaneous output (R) */
#define MGA_VGA_SEQ_INDEX	0x1FC4	/* Sequencer register index */
#define MGA_VGA_SEQ_DATA	0x1FC5	/* Sequencer register data */
#define MGA_VGA_CRTC_INDEX	0x1FD4	/* CRTC register index */
#define MGA_VGA_CRTC_DATA	0x1FD5	/* CRTC register data */
#define MGA_VGA_STATUS1		0x1FDA	/* Input status register 1 (R) */
#define MGA_CRTCEXT_INDEX	0x1FDE	/* CRTC extension register index */
#define MGA_CRTCEXT_DATA	0x1FDF	/* CRTC extension register data */

#define MGA_CRTCEXT3_MGAMODE	0x80	/* enable MGA accelerator mode */

#define MGA_DAC_BASE		0x3C00	/* RAMDAC base offset in MGABASE1 */

/* TVP3026 direct register byte offsets from MGA_DAC_BASE */
#define MGA_DAC_PALADDR_W	0x00	/* palette write address */
#define MGA_DAC_PALDATA		0x01	/* palette data (R/W) */
#define MGA_DAC_PIXRDMSK	0x02	/* pixel read mask */
#define MGA_DAC_PALADDR_R	0x03	/* palette read address */

/* Indirect register access (overlay on direct registers) */
#define MGA_DAC_IND_INDEX	0x00	/* write indirect register index here */
#define MGA_DAC_IND_DATA	0x0A	/* read/write indirect register data */

#define MGA_TVP_COLORMODE	0x0F	/* LATCH_CTL: 0x07=truecolor 0x06=8bpp */
#define MGA_TVP_PIXFMT		0x18	/* TRUE_COLOR_CTL: 0x05=16bpp bypass, 0x80=8bpp palette */
#define MGA_TVP_CURCTL		0x19	/* MUX_CTL: 0x54/0x53=16bpp 64/32-bit, 0x4C/0x4B=8bpp */
#define MGA_TVP_MUXCTL		0x1A	/* CLK_SEL: 0x15=16bpp, 0x25=8bpp */
#define MGA_TVP_VSYNCPOL	0x1D	/* vsync polarity: 0x03 for >= 768 lines */
#define MGA_TVP_LUTBYPASS	0x1E	/* LUT bypass / MISC_CTL: 0x24=truecolor, 0x0C=palette */
#define MGA_TVP_KEY_CTL		0x38	/* color key control: 0x00 = disable */

#define MGA_TVP_PLLADDR		0x2C	/* PCLK PLL address pointer */
#define MGA_TVP_PLLDATA		0x2D	/* PCLK PLL N/M/P data (auto-increments) */
#define MGA_TVP_MEMPLLDATA	0x2E	/* MCLK PLL N/M/P data (auto-increments) */
#define MGA_TVP_LOOPPLLDATA	0x2F	/* Loop clock PLL data */
#define MGA_TVP_MEMPLLCTRL	0x39	/* MCLK PLL control */

/* Special PLLADDR values */
#define MGA_TVP_PLLADDR_PCLK_START	0x00	/* start PCLK sequence */
#define MGA_TVP_PLLADDR_MCLK_START	0xF3	/* start MCLK PLL sequence */
#define MGA_TVP_PLLADDR_MCLK_STOP	0xFB	/* stop MCLK PLL (PAR[3:2]=10=P, write 0x00) */
#define MGA_TVP_PLLADDR_PCLK_N		0xFC	/* read-back PCLK N (PAR[1:0]=00) */
#define MGA_TVP_PLLADDR_PCLK_M		0xFD	/* read-back PCLK M (PAR[1:0]=01) */
#define MGA_TVP_PLLADDR_PCLK_STOP	0xFE	/* stop PCLK: PAR[1:0]=10=P, write 0x00 */
#define MGA_TVP_PLLADDR_ALL_STATUS	0x3F	/* all PLL pointers → status */

#define MGA_TVP_GEN_IO_CTL	0x2A	/* GPIO driver enable: 1=drive LOW */
#define MGA_TVP_GEN_IO_DATA	0x2B	/* GPIO read-back */
#define MGA_DDC_SDA		0x04	/* SDA = GPIO bit 2 */
#define MGA_DDC_SCL		0x10	/* SCL = GPIO bit 4 */

/* TVP3026 direct registers for hardware cursor */
#define MGA_DAC_CUR_COL_ADDR_W	0x04	/* cursor/overscan color write addr */
#define MGA_DAC_CUR_COL_DATA	0x05	/* cursor color data (R,G,B seq.) */
#define MGA_DAC_CUR_CTRL	0x09	/* direct cursor control */
#define MGA_DAC_CUR_DATA	0x0B	/* cursor RAM data (auto-incr) */
#define MGA_DAC_CUR_X_LSB	0x0C	/* cursor X position LSB */
#define MGA_DAC_CUR_X_MSB	0x0D	/* cursor X position MSB */
#define MGA_DAC_CUR_Y_LSB	0x0E	/* cursor Y position LSB */
#define MGA_DAC_CUR_Y_MSB	0x0F	/* cursor Y position MSB */

/* TVP3026 indirect register 0x06: cursor control */
#define MGA_TVP_CURCTL_IND	0x06
#define MGA_TVP_CURCTL_XGA	0x02	/* XGA mode (complement + transparent) */
#define MGA_TVP_CURCTL_XWIN	0x03	/* X-windows mode (2-color + transparent) */
#define MGA_TVP_CURCTL_OFF	0x00	/* cursor disabled */
#define MGA_TVP_CURCTL_CMASK	0x03	/* mode bits 1:0 */
#define MGA_TVP_CURCTL_BMASK	0x0C	/* bank bits 3:2 (CCR3:CCR2) */
#define MGA_TVP_CURCTL_BSHIFT	2
#define MGA_TVP_CURCTL_CCR7	0x80	/* must clear for indirect control */

#define MGA_CURSOR_MAX		64	/* max width and height in pixels */
#define MGA_CURSOR_PLANE_SIZE	512	/* bytes per plane (64 * 64 / 8) */
#define MGA_CURSOR_ORIGIN	64	/* position register origin offset */

#define MGA_TVP_MEMPLLCTRL_STROBEMKC4		0x08	/* latch routing change */
#define MGA_TVP_MEMPLLCTRL_MCLK_PIXPLL		0x00	/* route PCLK to MCLK */
#define MGA_TVP_MEMPLLCTRL_MCLK_MCLKPLL	0x30	/* route MCLK PLL to MCLK */

#define MGA_TVP_PLL_LOCKED	0x40

#define MGA_TVP_REFCLK		14318

#define MGA_PCI_OPTION		0x40
#define MGA_OPTION_INTERLEAVE	0x00001000UL	/* WRAM interleave (bit 12) */
#define MGA_OPTION_M_RESET	0x00100000UL	/* WRAM controller reset (bit 20) */
#define MGA_OPTION_RFHCNT_SHIFT	16
#define MGA_OPTION_RFHCNT_MASK	0x000F0000UL
#define MGA_OPTION_BIOSEN	0x40000000UL	/* BIOS ROM enable (bit 30) */
#define MGA_OPTION_POWERPC	0x80000000UL	/* big-endian host register swap */

/* Minimum MCLK for the Matrox Millennium (MGA-2064W). */
#define MGA_MCLK_KHZ		50000

#define MGA_PW8		0x00
#define MGA_PW16	0x01
#define MGA_PW32	0x02

#define MGA_OPMODE_DIRDATASIZ_8		0x00000000UL	/*  8 bpp */
#define MGA_OPMODE_DIRDATASIZ_16	0x00000001UL	/* 16 bpp */
#define MGA_OPMODE_DIRDATASIZ_32	0x00000002UL	/* 32 bpp */
#define MGA_OPMODE_DIRDATASIZ_MASK	0x00000003UL

#define MGA_OPMODE_DMA_BLIT_WR	0x00000005UL	/* dmamod=BLIT_WRITE + enable */
#define MGA_OPMODE_DMA_OFF	0x00000000UL	/* restore default */

/*
 * MGA-1064SG (Mystique) integrated DAC indirect register indices.
 * Accessed via the same 0x3C00 base (MGA_DAC_IND_INDEX / MGA_DAC_IND_DATA).
 */
#define MGA_IDAC_VREF_CTL	0x18	/* voltage reference control */
#define MGA_IDAC_MUL_CTL	0x19	/* pixel depth control */
#define MGA_IDAC_PIX_CLK_CTL	0x1A	/* pixel clock source + enable */
#define MGA_IDAC_GEN_CTL	0x1D	/* general control */
#define MGA_IDAC_MISC_CTL	0x1E	/* DAC power, VGA compat */

/* MUL_CTL depth values */
#define MGA_MULCTL_8BPP		0x00
#define MGA_MULCTL_15BPP	0x01
#define MGA_MULCTL_16BPP	0x02	/* 5:6:5 */
#define MGA_MULCTL_24BPP	0x03
#define MGA_MULCTL_32BPP	0x04

/* PIX_CLK_CTL field values */
#define MGA_PIXCLK_SRC_PCI	0x00	/* pixel clock = PCI clock */
#define MGA_PIXCLK_SRC_PLL	0x01	/* pixel clock = pixel PLL */
#define MGA_PIXCLK_DISABLE	0x04	/* disable pixel clock output */
#define MGA_PIXPLL_POWERDOWN	0x08	/* power down pixel PLL */

/* System PLL registers (1064SG integrated DAC) */
#define MGA_IDAC_SYS_PLL_M	0x2C
#define MGA_IDAC_SYS_PLL_N	0x2D
#define MGA_IDAC_SYS_PLL_P	0x2E
#define MGA_IDAC_SYS_PLL_STAT	0x2F

/* Pixel PLL set C registers (1064SG — xf86 uses set C) */
#define MGA_IDAC_PIX_PLLC_M	0x4C
#define MGA_IDAC_PIX_PLLC_N	0x4D
#define MGA_IDAC_PIX_PLLC_P	0x4E
#define MGA_IDAC_PIX_PLL_STAT	0x4F

/* 1064SG OPTION register bits (PCI config 0x40) — DIFFERENT from 2064W */
#define MGA1064_OPT_SYSCLK_PCI		0x00000000UL
#define MGA1064_OPT_SYSCLK_PLL		0x00000001UL
#define MGA1064_OPT_SYSCLK_MASK	0x00000003UL
#define MGA1064_OPT_SYSCLK_DIS		0x00000004UL
#define MGA1064_OPT_G_CLK_DIV_1	0x00000008UL
#define MGA1064_OPT_M_CLK_DIV_1	0x00000010UL
#define MGA1064_OPT_SYS_PLL_PDN	0x00000020UL
#define MGA1064_OPT_VGA_IOEN		0x00000100UL

/* 1064SG reference clock frequency (14.31818 MHz) */
#define MGA_IDAC_REFCLK		14318

/* 1064SG hardcoded OPTION value from xf86-video-mga MGAGInit() */
#define MGA1064_OPTION_DEFAULT	0x5F094F21UL

#endif /* MGAFBREG_H */
