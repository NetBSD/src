/*	$NetBSD: veritefbreg.h,v 1.1 2026/07/11 15:18:21 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

/*
 * Register definitions for the Rendition Verite.
 */

#ifndef VERITEFBREG_H
#define VERITEFBREG_H

#define VFB_IO_BAR		0x14	/* I/O registers */
#define VFB_FB_BAR		0x10	/* linear framebuffer aperture */
#define VFB_MMIO_BAR		0x18	/* MMIO register/FIFO window */

#define VFB_MMIO_REG_BASE	0x20000

/*
 * Input FIFO windows: 32-bit writes, byte-swap variant selected by
 * address.
 */
#define VFB_FIFO_SWAP_NO	0x00	/* no byte swap */
#define VFB_FIFO_SWAP_END	0x04	/* swap bytes 3<>0, 2<>1 */
#define VFB_FIFO_SWAP_INHW	0x08	/* swap bytes 3<>2, 1<>0 */
#define VFB_FIFO_SWAP_HW	0x0c	/* swap half-words */

#define VFB_FIFOINFREE		0x40	/* input FIFO free entry count */
#define VFB_FIFOINFREE_MASK	0x1f
#define VFB_FIFO_SIZE		0x1f
#define VFB_FIFOOUTVALID	0x41	/* output FIFO valid entry count */
#define VFB_FIFOOUTVALID_MASK	0x07

#define VFB_COMM		0x42	/* dual 4-bit comm ports */
#define VFB_COMM_SYSSTATUS_MASK	0x0f	/* host -> RISC */
#define VFB_COMM_RISCSTATUS_MASK 0xf0	/* RISC -> host, r/o */

#define VFB_MEMENDIAN		0x43	/* aperture byte-swap policy */
#define VFB_MEMENDIAN_NO	0	/* no byte swap */
#define VFB_MEMENDIAN_END	1	/* swap bytes 3<>0, 2<>1 */
#define VFB_MEMENDIAN_INHW	2	/* swap bytes 3<>2, 1<>0 */
#define VFB_MEMENDIAN_HW	3	/* swap half-words */
#define VFB_MEMENDIAN_MASK	3

#define VFB_INTR		0x44	/* interrupt status */
#define VFB_INTREN		0x46	/* interrupt enable */
#define VFB_INTR_VERT		0x01	/* vertical retrace */
#define VFB_INTR_FIFOLOW	0x02	/* input FIFO above low water */
#define VFB_INTR_RISC		0x04	/* RISC firmware interrupt */
#define VFB_INTR_HALT		0x08	/* RISC halted */
#define VFB_INTR_FIFOERROR	0x10	/* FIFO under/overflow */
#define VFB_INTR_DMAERROR	0x20	/* PCI error during DMA */
#define VFB_INTR_DMA		0x40	/* DMA done */
#define VFB_INTR_X		0x80	/* external device passthrough */

#define VFB_DEBUG		0x48	/* reset and RISC debug control */
#define VFB_DEBUG_SOFTRESET	0x01	/* soft reset chip */
#define VFB_DEBUG_HOLDRISC	0x02	/* hold RISC while set */
#define VFB_DEBUG_STEPRISC	0x04	/* single-step RISC */
#define VFB_DEBUG_DIRECTSCLK	0x08	/* no divide-by-2 for sys clock */
#define VFB_DEBUG_SOFTVGARESET	0x10	/* assert VGA reset */
#define VFB_DEBUG_SOFTXRESET	0x20	/* assert XReset to ext devices */

#define VFB_LOWWATERMARK	0x49	/* input FIFO low water mark */

#define VFB_STATUS		0x4a	/* V2x00 only: busy blocks */
#define VFB_STATUS_HOLD_MASK	0x8c	/* must all be set before hold */
#define VFB_STATUS_HELD		0x02	/* mirrors the Debug hold bit */

#define VFB_XBUSCTL		0x4b	/* V2x00 only */

#define VFB_DMACMDPTR		0x50	/* DMA command list pointer */
#define VFB_DMAADDRESS		0x54	/* DMA data address */
#define VFB_DMACOUNT		0x58	/* DMA remaining transfer count */

/* RISC state access window (debug port). */
#define VFB_STATEINDEX		0x60
#define VFB_STATEDATA		0x64
#define VFB_STATEINDEX_IR	128	/* decoder instruction register */
#define VFB_STATEINDEX_PC	129	/* program counter */
#define VFB_STATEINDEX_S1	130	/* S1 operand bus */

#define VFB_SCLKPLL		0x68	/* V2x00 only: system clock PLL */

#define VFB_SCRATCH		0x70	/* 16-bit BIOS scratch space */

#define VFB_MODE		0x72	/* legacy VGA vs native mode */
#define VFB_MODE_VESA		0x01	/* enable 0xA0000 in native mode */
#define VFB_MODE_VGA		0x02	/* VGA mode if set, else native */
#define VFB_MODE_VGA32		0x04	/* enable VGA 32-bit accesses */
#define VFB_MODE_DMAEN		0x08	/* enable DMA accesses */
#define VFB_MODE_NATIVE		0x00	/* not VESA and not VGA */

#define VFB_BANKSELECT		0x74	/* local memory to 0xA0000 mapping */

/* CRTC */
#define VFB_CRTCTEST		0x80
#define VFB_CRTCTEST_VIDEOLATENCY_MASK	0x1f
#define VFB_CRTCTEST_NOTVBLANK	0x10000
#define VFB_CRTCTEST_VBLANK	0x40000

#define VFB_CRTCCTL		0x84
#define VFB_CRTCCTL_SCRNFMT_MASK	0xf
#define VFB_CRTCCTL_VIDEOFIFOSIZE128	0x10
#define VFB_CRTCCTL_ENABLEDDC	0x20
#define VFB_CRTCCTL_DDCOUTPUT	0x40
#define VFB_CRTCCTL_DDCDATA	0x80
#define VFB_CRTCCTL_VSYNCHI	0x100
#define VFB_CRTCCTL_HSYNCHI	0x200
#define VFB_CRTCCTL_VSYNCENABLE	0x400
#define VFB_CRTCCTL_HSYNCENABLE	0x800
#define VFB_CRTCCTL_VIDEOENABLE	0x1000
#define VFB_CRTCCTL_STEREOSCOPIC	0x2000
#define VFB_CRTCCTL_FRAMEDISPLAYED	0x4000
#define VFB_CRTCCTL_FRAMEBUFFERBGR	0x8000
#define VFB_CRTCCTL_EVENFRAME	0x10000
#define VFB_CRTCCTL_LINEDOUBLE	0x20000
#define VFB_CRTCCTL_FRAMESWITCHED	0x40000
#define VFB_CRTCCTL_VIDEOFIFOSIZE256	0x800000 /* V2x00 only */

/*
 * CRTC horizontal timing
 */
#define VFB_CRTCHORZ		0x88
#define VFB_CRTCHORZ_ACTIVE_MASK	0xff
#define VFB_CRTCHORZ_BACKPORCH_MASK	0x7e00
#define VFB_CRTCHORZ_SYNC_MASK		0x1f0000
#define VFB_CRTCHORZ_FRONTPORCH_MASK	0xe00000

/* CRTC vertical timing */
#define VFB_CRTCVERT		0x8c
#define VFB_CRTCVERT_ACTIVE_MASK	0x7ff
#define VFB_CRTCVERT_BACKPORCH_MASK	0x1f800
#define VFB_CRTCVERT_SYNC_MASK		0xe0000
#define VFB_CRTCVERT_FRONTPORCH_MASK	0x03f00000

#define VFB_FRAMEBASEB		0x90	/* stereoscopic frame base B */
#define VFB_FRAMEBASEA		0x94	/* frame base A */

#define VFB_CRTCOFFSET		0x98
#define VFB_CRTCOFFSET_MASK	0xffff
#define VFB_VIDEOFIFO_BYTES	128	/* with CRTCCTL VIDEOFIFOSIZE128 */
#define VFB_CRTCSTATUS		0x9c	/* video scan position */
#define VFB_CRTCSTATUS_VERT_MASK	0xc00000
#define VFB_CRTCSTATUS_VERT_FPORCH	0x400000
#define VFB_CRTCSTATUS_VERT_SYNC	0xc00000
#define VFB_CRTCSTATUS_VERT_BPORCH	0x800000
#define VFB_CRTCSTATUS_VERT_ACTIVE	0x000000

/*
 * Memory controller
 */
#define VFB_MEMCTL		0xa0
#define VFB_MEMCTL_ADRSWIZZLE_MASK	0x1800
#define VFB_MEMCTL_HOLDREFRESH		0x2000
#define VFB_MEMCTL_WREFRESH_MASK	0xff0000
#define VFB_MEMCTL_WREFRESH_DEFAULT	0x330000

#define VFB_MEMDIAG		0xa4	/* V2x00 only */
#define VFB_CURSORBASE		0xac	/* V2x00 only: bits [23:10] */

/*
 * Start of the register block that exists ONLY in PCI I/O space
 */
#define VFB_IOONLY_BASE		0xb0

/* RAMDAC, byte-wide registers */
#define VFB_DACRAMWRITEADR	0xb0
#define VFB_DACRAMDATA		0xb1
#define VFB_DACPIXELMSK		0xb2
#define VFB_DACRAMREADADR	0xb3
#define VFB_DACOVSWRITEADR	0xb4
#define VFB_DACOVSDATA		0xb5
#define VFB_DACCOMMAND0		0xb6
#define VFB_DACOVSREADADR	0xb7
#define VFB_DACCOMMAND1		0xb8
#define VFB_DACCOMMAND2		0xb9
#define VFB_DACSTATUS		0xba
#define VFB_DACCOMMAND3		0xba	/* via unlocking/indexing */
#define VFB_DACCURSORDATA	0xbb
#define VFB_DACCURSORXLOW	0xbc
#define VFB_DACCURSORXHIGH	0xbd
#define VFB_DACCURSORYLOW	0xbe
#define VFB_DACCURSORYHIGH	0xbf

/*
 * Pixel clock PLL (V2x00)
 */
#define VFB_PCLKPLL		0xc0
#define VFB_PCLKPLL_M_MASK	__BITS(8, 0)
#define VFB_PCLKPLL_P_MASK	__BITS(12, 9)
#define VFB_PCLKPLL_N_MASK	__BITS(18, 13)
/* divider search bounds (the reference driver stays below field max) */
#define VFB_PLL_M_MAX		0xff
#define VFB_PLL_N_MAX		0x3f
#define VFB_PLL_P_MAX		0x0f
/* the constraints above, in units of 10 Hz */
#define VFB_PLL_PCF_MIN		100000		/* 1 MHz */
#define VFB_PLL_PCF_MAX		300000		/* 3 MHz */
#define VFB_PLL_VCO_MIN		12500000	/* 125 MHz */
#define VFB_PLL_VCO_MAX		25000000	/* 250 MHz */
#define VFB_PLL_STABILIZE_US	500		/* spec: 200 us */

/*
 * System/memory clock PLL (V2x00)
 */
#define VFB_SCLKPLL_DEFAULT	0xa484d

/* CRTCCTL ScrnFmt pixel format codes */
#define VFB_PIXFMT_332		1
#define VFB_PIXFMT_8I		2	/* 8bpp indexed */
#define VFB_PIXFMT_565		4
#define VFB_PIXFMT_4444		5
#define VFB_PIXFMT_1555		6
#define VFB_PIXFMT_8888		12

/*
 * RAMDAC command register bits (Bt485-compatible core)
 */
#define VFB_DACCMD0_EXTENDED	0x80	/* enable cmd3 access */
#define VFB_DACCMD0_8BITDAC	0x02
#define VFB_DACCMD1_24BPP	0x00
#define VFB_DACCMD1_16BPP	0x20
#define VFB_DACCMD1_8BPP	0x40
#define VFB_DACCMD1_BYPASS_CLUT	0x10
#define VFB_DACCMD1_565		0x08
#define VFB_DACCMD1_PORT_AB	0x00
#define VFB_DACCMD2_PIXEL_INPUT_GATE	0x20
#define VFB_DACCMD2_DISABLE_CURSOR	0x00
#define VFB_DACCMD2_CURSOR_MASK	0x03
#define VFB_DACCMD3_INDEX	0x01	/* index via DACRAMWRITEADR */
#define VFB_DACCMD3_CLK_DOUBLER	0x08

#endif /* VERITEFBREG_H */
