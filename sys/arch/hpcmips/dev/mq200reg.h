/*	$NetBSD: mq200reg.h,v 1.1.2.2 2000/08/06 03:56:42 takemura Exp $	*/

/*-
 * Copyright (c) 2000 Takemura Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define MQ200_VENDOR_ID		0x4d51
#define MQ200_PRODUCT_ID	0x0200
#define MQ200_MAPSIZE		0x800000

#define MQ200_POWERSTATE_D0	0
#define MQ200_POWERSTATE_D1	1
#define MQ200_POWERSTATE_D2	2
#define MQ200_POWERSTATE_D3	3

#define MQ200_FRAMEBUFFER	0x000000	/* frame buffer base address */
#define MQ200_PM		0x600000	/* power management	*/
#define MQ200_CC		0x602000	/* CPU interface	*/
#define MQ200_MM		0x604000	/* memory interface unit */
#define MQ200_IN		0x608000	/* interrupt controller	*/
#define MQ200_GC(n)		(0x60a000+0x80*(n))
#define MQ200_GC1		0x60a000	/* graphice controller 1*/
#define MQ200_GC2		0x60a080	/* graphice controller 1*/
#define MQ200_GE		0x60c000	/* graphics engine	*/
#define MQ200_FP		0x60e000	/* flat panel controller*/
#define MQ200_CP1		0x610000	/* color palette 1	*/
#define MQ200_DC		0x614000	/* device configration	*/
#define MQ200_PC		0x616000	/* PCI configration	*/

/*
 * Power Management
 */

/*
 * CPU Interface
 */

/*
 * Memory Interface Unit
 */

/*
 * Interrupt Controller
 */

/*
 * Graphics Controller 1/2
 */
/* GC Control (index: 00h)	*/
#define MQ200_GCCR(n)		(MQ200_GC(n)+0x00)
#	define MQ200_GCC_ENABLE		(1<<0)
#	define MQ200_GCC_HCRESET	(1<<1)
#	define MQ200_GCC_VCRESET	(1<<2)
#	define MQ200_GCC_EN		(1<<3)
#	define MQ200_GCC_DEPTH_SHIFT	4
#	define MQ200_GCC_DEPTH_MASK	0x000000f0
#	define MQ200_GCC_CSREN		(1<<8)
	/* bits 10-9 is reserved */
#	define MQ200_GCC_ALTEN		(1<<11)
#	define MQ200_GCC_ALTDEPTH_SHIFT 12
#	define MQ200_GCC_ALTDEPTH_MASK	0x0000f000
#	define MQ200_GCC_RCLK_MASK	0x00030000
#	define MQ200_GCC_RCLK_BUS	0x00000000
#	define MQ200_GCC_RCLK_PLL1	0x00010000
#	define MQ200_GCC_RCLK_PLL2	0x00020000
#	define MQ200_GCC_RCLK_PLL3	0x00030000
#	define MQ200_GCC_TESTMODE0	(1<<18)
#	define MQ200_GCC_TESTMODE1	(1<<19)
	/* FD(first clock divisor) is 1, 1.5, 2.5, 3.5, 4.5, 5.6 or 6.5 */
#	define MQ200_GCC_MCLK_FD_MASK	0x00700000
#	define MQ200_GCC_MCLK_FD_1	0x00000000
#	define MQ200_GCC_MCLK_FD_1_5	0x00100000
#	define MQ200_GCC_MCLK_FD_2_5	0x00200000
#	define MQ200_GCC_MCLK_FD_3_5	0x00300000
#	define MQ200_GCC_MCLK_FD_4_5	0x00400000
#	define MQ200_GCC_MCLK_FD_5_5	0x00500000
#	define MQ200_GCC_MCLK_FD_6_5	0x00600000
	/* bit 23 is reserved */
	/* SD(second close divisor) is 1-255. 0 means disable */
#	define MQ200_GCC_MCLK_SD_SHIFT	24
#	define MQ200_GCC_MCLK_SD_MASK	0xff000000
	/* GCCR_DEPTH and GCCR_ALTDEPTH values */
#	define MQ200_GCC_1BPP		0x0
#	define MQ200_GCC_2BPP		0x1
#	define MQ200_GCC_4BPP		0x2
#	define MQ200_GCC_8BPP		0x3
#	define MQ200_GCC_16BPP		0x4
#	define MQ200_GCC_24BPP		0x5
#	define MQ200_GCC_ARGB888	0x6
#	define MQ200_GCC_PALBGR		0x6
#	define MQ200_GCC_ABGR888	0x7
#	define MQ200_GCC_PALRGB		0x7
#	define MQ200_GCC_16BPP_DIRECT	0xc
#	define MQ200_GCC_24BPP_DIRECT	0xd
#	define MQ200_GCC_ARGB888_DIRECT 0xe
#	define MQ200_GCC_PALBGR_DIRECT	0xe
#	define MQ200_GCC_ABGR888_DIRECT 0xf
#	define MQ200_GCC_PALRGB_DIRECT	0xf

/* GC CRT Control (index: 04h)	*/
#define MQ200_GCCRTCR(n)	(MQ200_GC(n)+0x04)
#	define MQ200_GCCRTC_DACEN		(1<<0)
#	define MQ200_GCCRTC_HSYNC_PMCLK		(1<<2)
#	define MQ200_GCCRTC_VSYNC_PMCLK		(1<<3)
#	define MQ200_GCCRTC_HSYNC_LOW		0x00000010
#	define MQ200_GCCRTC_HSYNC_HIGH		0x00000020
#	define MQ200_GCCRTC_VSYNC_LOW		0x00000040
#	define MQ200_GCCRTC_VSYNC_HIGH		0x00000080
#	define MQ200_GCCRTC_HSYNC_ACTVHIGH	(0<<8)
#	define MQ200_GCCRTC_HSYNC_ACTVLOW	(1<<8)
#	define MQ200_GCCRTC_VSYNC_ACTVHIGH	(0<<9)
#	define MQ200_GCCRTC_VSYNC_ACTVLOW	(1<<9)
#	define MQ200_GCCRTC_SYNC_PEDESTAL_EN	(1<<10)
#	define MQ200_GCCRTC_BLANK_PEDESTAL_EN	(1<<11)
#	define MQ200_GCCRTC_COMPOSITE_SYNC_EN	(1<<12)
#	define MQ200_GCCRTC_VREF_INTR		(0<<13)
#	define MQ200_GCCRTC_VREF_EXTR		(1<<13)
#	define MQ200_GCCRTC_MONITOR_SENCE_EN	(1<<14)
#	define MQ200_GCCRTC_CONSTAND_OUTPUT_EN	(1<<15)
#	define MQ200_GCCRTC_OUTPUT_LEVEL_MASK	0x00ff0000
#	define MQ200_GCCRTC_OUTPUT_LEVEL_SHIFT	16
#	define MQ200_GCCRTC_BLUE_NOTLOADED	(1<<24)
#	define MQ200_GCCRTC_RED_NOTLOADED	(1<<25)
#	define MQ200_GCCRTC_GREEN_NOTLOADED	(1<<26)
	/* bit 27 is reserved */
#	define MQ200_GCCRTC_COLOR		(0<<28)
#	define MQ200_GCCRTC_MONO		(1<<28)
	/* bits 31-29 are reserved */

/* GC Hotizontal Display Control (index: 08h)	*/
#define MQ200_GCHDCR(n)		(MQ200_GC(n)+0x08)
#	define MQ200_GCHDC_TOTAL_MASK		0x00000fff
#	define MQ200_GCHDC_TOTAL_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHDC_END_MASK		0x0fff0000
#	define MQ200_GCHDC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Display Control (index: 0Ch)	*/
#define MQ200_GCVDCR(n)		(MQ200_GC(n)+0x0c)
#	define MQ200_GCVDC_TOTAL_MASK		0x00000fff
#	define MQ200_GCVDC_TOTAL_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCVDC_END_MASK		0x0fff0000
#	define MQ200_GCVDC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Hotizontal Sync Control (index: 10h)	*/
#define MQ200_GCHSCR(n)		(MQ200_GC(n)+0x10)
#	define MQ200_GCHSC_START_MASK		0x00000fff
#	define MQ200_GCHSC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHSC_END_MASK		0x0fff0000
#	define MQ200_GCHSC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Sync Control (index: 14h)	*/
#define MQ200_GCVSCR(n)		(MQ200_GC(n)+0x14)
#	define MQ200_GCVSC_START_MASK		0x00000fff
#	define MQ200_GCVSC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCVSC_END_MASK		0x0fff0000
#	define MQ200_GCVSC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Display Count (index: 1Ch)	*/
#define MQ200_GCVDCNTR(n)	(MQ200_GC(n)+0x1c)
#	define MQ200_GCVDCNT_MASK		0x00000fff
	/* bits 31-12 are reserved */

/* GC Horizontal Window Control (index: 20h)	*/
#define MQ200_GCHWCR(n)		(MQ200_GC(n)+0x20)
#	define MQ200_GCHWC_START_MASK		0x00000fff
#	define MQ200_GCHWC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHWC_WIDTH_MASK		0x0fff0000
#	define MQ200_GCHWC_WIDTH_SHIFT		16
	/* ALD: Additional Line Delta */
#	define MQ200_GCHWC_ALD_MASK		0xf0000000
#	define MQ200_GCHWC_ALD_SHIFT		28

/* GC Vertical Window Control (index: 24h)	*/
#define MQ200_GCVWCR(n)		(MQ200_GC(n)+0x24)
#	define MQ200_GCVWC_START_MASK		0x00000fff
#	define MQ200_GCVWC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCVWC_HEIGHT_MASK		0x0fff0000
#	define MQ200_GCVWC_HEIGHT_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Altarnate Horizontal Window Control (index: 28h)	*/
#define MQ200_GCHAWCR(n)	(MQ200_GC(n)+0x28)
#	define MQ200_GCAHWC_START_MASK		0x00000fff
#	define MQ200_GCAHWC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCAHWC_WIDTH_MASK		0x0fff0000
#	define MQ200_GCAHWC_WIDTH_SHIFT		16
	/* ALD: Additional Line Delta */
#	define MQ200_GCAHWC_ALD_MASK		0xf0000000
#	define MQ200_GCAHWC_ALD_SHIFT		28

/* GC Alternate Vertical Window Control (index: 2Ch)	*/
#define MQ200_GCAVWCR(n)	(MQ200_GC(n)+0x2C)
#	define MQ200_GCAVWC_START_MASK		0x00000fff
#	define MQ200_GCAVWC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCAVWC_HEIGHT_MASK		0x0fff0000
#	define MQ200_GCAVWC_HEIGHT_SHIFT	16
	/* bits 31-28 are reserved */

/* GC Window Start Address (index: 30h)	*/
#define MQ200_GCWSAR(n)		(MQ200_GC(n)+0x30)
#	define MQ200_GCWSA_MASK		0x000fffff
	/* bits 31-21 are reserved */

/* GC Alternate Window Start Address (index: 34h)	*/
#define MQ200_GCAWSAR(n)	(MQ200_GC(n)+0x34)
#	define MQ200_GCAWSA_MASK	0x000fffff
	/* bits 24-21 are reserved */
#	define MQ200_GCAWPI_MASK	0xfe000000
#	define MQ200_GCAWPI_SHIFT	24	/* XXX, 24 could be usefull
						   than 23 */

/* GC Window Stride (index: 38h)	*/
#define MQ200_GCWSTR(n)		(MQ200_GC(n)+0x38)
#	define MQ200_GCWST_MASK		0x0000ffff
#	define MQ200_GCWST_SHIFT	0
#	define MQ200_GCWST_ALTMASK	0xffff0000
#	define MQ200_GCWST_ALTSHIFT	16

/* GC Hardware Cursor Position (index: 40h)	*/
#define MQ200_GCHCPR(n)		(MQ200_GC(n)+0x40)
#	define MQ200_GCHCP_HSTART_MASK		0x00000fff
#	define MQ200_GCHCP_HSTART_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHCP_VSTART_MASK		0x0fff0000
#	define MQ200_GCHCP_VSTART_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Hardware Start Address and Offset (index: 44h)	*/
#define MQ200_GCHCAOR(n)		(MQ200_GC(n)+0x44)
#	define MQ200_GCHCAO_ADDR_MASK		0x00000fff
#	define MQ200_GCHCAO_ADDR_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHCAO_HOFFSET_MASK	0x003f0000
#	define MQ200_GCHCAO_HOFFSET_SHIFT	16
	/* bits 23-22 are reserved */
#	define MQ200_GCHCAO_VOFFSET_MASK	0x3f000000
#	define MQ200_GCHCAO_VOFFSET_SHIFT	24
	/* bits 31-30 are reserved */

/* GC Hardware Cursor Foreground Color (index: 48h)	*/
#define MQ200_GCHCFCR(n)	(MQ200_GC(n)+0x48)
#	define MQ200_GCHCFC_MASK		0x00ffffff
	/* you can use MQ200_GC_RGB macro	*/
	/* bits 31-24 are reserved */

/* GC Hardware Cursor Background Color (index: 4Ch)	*/
#define MQ200_GCHCBCR(n)	(MQ200_GC(n)+0x4c)
#	define MQ200_GCHCBC_MASK		0x00ffffff
	/* you can use MQ200_GC_RGB macro	*/
	/* bits 31-24 are reserved */

#define MQ200_GC1CR		MQ200_GCCR(0)
#define MQ200_GC1CRTCR		MQ200_GCCRTCR(0)
#define MQ200_GC1HDCR		MQ200_GCHDCR(0)
#define MQ200_GC1VDCR		MQ200_GCVDCR(0)
#define MQ200_GC1HSCR		MQ200_GCHSCR(0)
#define MQ200_GC1VSCR		MQ200_GCVSCR(0)
#define MQ200_GC1VDCNTR		MQ200_GCVDCNTR(0)
#define MQ200_GC1HWCR		MQ200_GCHWCR(0)
#define MQ200_GC1VWCR		MQ200_GCVWCR(0)
#define MQ200_GC1HAWCR		MQ200_GCHAWCR(0)
#define MQ200_GC1AVWCR		MQ200_GCAVWCR(0)
#define MQ200_GC1WSAR		MQ200_GCWSAR(0)
#define MQ200_GC1AWSAR		MQ200_GCAWSAR(0)
#define MQ200_GC1WSTR		MQ200_GCWSTR(0)
#define MQ200_GC1HCPR		MQ200_GCHCPR(0)
#define MQ200_GC1HCAOR		MQ200_GCHCAOR(0)
#define MQ200_GC1HCFCR		MQ200_GCHCFCR(0)
#define MQ200_GC1HCBCR		MQ200_GCHCBCR(0)

#define MQ200_GC2CR		MQ200_GCCR(1)
#define MQ200_GC2CRTCR		MQ200_GCCRTCR(1)
#define MQ200_GC2HDCR		MQ200_GCHDCR(1)
#define MQ200_GC2VDCR		MQ200_GCVDCR(1)
#define MQ200_GC2HSCR		MQ200_GCHSCR(1)
#define MQ200_GC2VSCR		MQ200_GCVSCR(1)
#define MQ200_GC2VDCNTR		MQ200_GCVDCNTR(1)
#define MQ200_GC2HWCR		MQ200_GCHWCR(1)
#define MQ200_GC2VWCR		MQ200_GCVWCR(1)
#define MQ200_GC2HAWCR		MQ200_GCHAWCR(1)
#define MQ200_GC2AVWCR		MQ200_GCAVWCR(1)
#define MQ200_GC2WSAR		MQ200_GCWSAR(1)
#define MQ200_GC2AWSAR		MQ200_GCAWSAR(1)
#define MQ200_GC2WSTR		MQ200_GCWSTR(1)
#define MQ200_GC2HCPR		MQ200_GCHCPR(1)
#define MQ200_GC2HCAOR		MQ200_GCHCAOR(1)
#define MQ200_GC2HCFCR		MQ200_GCHCFCR(1)
#define MQ200_GC2HCBCR		MQ200_GCHCBCR(1)

/*
 * Graphics Engine
 */

/*
 * Flat Pannel Controler
 */
/* FP Control	(index: 00h)	*/
#define MQ200_FPCR		(MQ200_FP + 0x00)
#	define MQ200_FPC_EN		(1<<0)
#	define MQ200_FPC_GC1		(0<<1)
#	define MQ200_FPC_GC2		(1<<1)
#	define MQ200_FPC_TFT		(0<<2)
#	define MQ200_FPC_SSTN		(1<<2)
#	define MQ200_FPC_DSTN		(2<<2)
#	define MQ200_FPC_MODE_MASK	0x000000e0
#	define MQ200_FPC_MODE_SHIFT	5
#	define MQ200_FPC_MONO		(1<<4)
#	define MQ200_FPC_TFT4MONO	(0<<5)
#	define MQ200_FPC_TFT12		(0<<5)
#	define MQ200_FPC_SSTN4		(0<<5)
#	define MQ200_FPC_DSTN8		(0<<5)
#	define MQ200_FPC_TFT6MONO	(1<<5)
#	define MQ200_FPC_TFT18		(1<<5)
#	define MQ200_FPC_SSTN8		(1<<5)
#	define MQ200_FPC_DSTN16		(1<<5)
#	define MQ200_FPC_TFT8MONO	(2<<5)
#	define MQ200_FPC_TFT24		(2<<5)
#	define MQ200_FPC_SSTN12		(2<<5)
#	define MQ200_FPC_DSTN24		(2<<5)
#	define MQ200_FPC_SSTN16		(3<<5)
#	define MQ200_FPC_SSTN24		(4<<5)
#	define MQ200_FPC_DITH_DISABLE	(0<<8)
#	define MQ200_FPC_DITH_PTRN1	(1<<8)
#	define MQ200_FPC_DITH_PTRN2	(2<<8)
#	define MQ200_FPC_DITH_PTRN3	(3<<8)
	/* bits 11-10 are reserved */
#	define MQ200_FPC_DITH_BC_MASK	0x00007000
#	define MQ200_FPC_DITH_BC_SHIFT	12
#	define MQ200_FPC_FRC_DISABLE_ALTWIN	(1<<15)
#	define MQ200_FPC_FRC_2LEVEL	(0<<16)
#	define MQ200_FPC_FRC_4LEVEL	(1<<16)
#	define MQ200_FPC_FRC_8LEVEL	(2<<16)
#	define MQ200_FPC_FRC_16LEVEL	(3<<16)
#	define MQ200_FPC_DITH_ADJ_MASK	0x0ffc0000
#	define MQ200_FPC_DITH_ADJ_SHIFT 18
#	define MQ200_FPC_DITH_ADJ_VAL	0x018
#	define MQ200_FPC_DITH_ADJ1_MASK	0x00fc0000
#	define MQ200_FPC_DITH_ADJ1_SHIFT 18
#	define MQ200_FPC_DITH_ADJ1_VAL	0x18
#	define MQ200_FPC_DITH_ADJ2_MASK	0x07000000
#	define MQ200_FPC_DITH_ADJ2_SHIFT 24
#	define MQ200_FPC_DITH_ADJ2_VAL	0x0
#	define MQ200_FPC_DITH_ADJ3_MASK	0x08000000
#	define MQ200_FPC_DITH_ADJ3_SHIFT 27
#	define MQ200_FPC_DITH_ADJ3_VAL	0x0
#	define MQ200_FPC_TESTMODE0	(1<<28)
#	define MQ200_FPC_TESTMODE1	(1<<29)
#	define MQ200_FPC_TESTMODE2	(1<<30)
#	define MQ200_FPC_TESTMODE3	(1<<31)

/* FP Output Pin Control	(index: 04h)	*/
#define MQ200_FPPCR		(MQ200_FP + 0x04)
#	define MQ200_FPPC_PIN_LOW	(1<<0)
#	define MQ200_FPPC_INVERSION_EN	(1<<1)
#	define MQ200_FPPC_FDE_COMPOSITE	(0<<2)
#	define MQ200_FPPC_FDE_HORIZONTAL (1<<2)
#	define MQ200_FPPC_FDE_FMOD_EN	(1<<3)
#	define MQ200_FPPC_FD2_DATAK	(0<<4)
#	define MQ200_FPPC_FD2_SHIFTCLK	(1<<4)
#	define MQ200_FPPC_FSCLK_EN	(1<<5)
#	define MQ200_FPPC_SHIFTCLK_DIV2	(1<<6)
#	define MQ200_FPPC_SHIFTCLK_MASK	(1<<7)
#	define MQ200_FPPC_STNLP_BLANK	(1<<8)
#	define MQ200_FPPC_SHIFTCLK_BLANK (1<<9)
#	define MQ200_FPPC_STNEXLP_EN	(1<<10)
	/* bit 11 is reserved */
#	define MQ200_FPPC_FD2_MAX	(0<<12)
#	define MQ200_FPPC_FD2_MID	(1<<12)
#	define MQ200_FPPC_FD2_MID2	(2<<12)
#	define MQ200_FPPC_FD2_MIN	(3<<12)
#	define MQ200_FPPC_DRV_MAX	(0<<12)
#	define MQ200_FPPC_DRV_MID	(1<<12)
#	define MQ200_FPPC_DRV_MID2	(2<<12)
#	define MQ200_FPPC_DRV_MIN	(3<<12)
#	define MQ200_FPPC_FD2_ACTVHIGH	(0<<16)
#	define MQ200_FPPC_FD2_ACTVLOW	(1<<16)
#	define MQ200_FPPC_ACTVHIGH	(0<<17)
#	define MQ200_FPPC_ACTVLOW	(1<<17)
#	define MQ200_FPPC_FDE_ACTVHIGH	(0<<18)
#	define MQ200_FPPC_FDE_ACTVLOW	(1<<18)
#	define MQ200_FPPC_FHSYNC_ACTVHIGH (0<<19)
#	define MQ200_FPPC_FHSYNC_ACTVLOW (1<<19)
#	define MQ200_FPPC_FVSYNC_ACTVHIGH (0<<20)
#	define MQ200_FPPC_FVSYNC_ACTVLOW (1<<20)
#	define MQ200_FPPC_FSCLK_ACTVHIGH (0<<21)
#	define MQ200_FPPC_FSCLK_ACTVLOW	(1<<21)
#	define MQ200_FPPC_FSCLK_MAX	(0<<22)
#	define MQ200_FPPC_FSCLK_MID	(1<<22)
#	define MQ200_FPPC_FSCLK_MID2	(2<<22)
#	define MQ200_FPPC_FSCLK_MIN	(3<<22)
#	define MQ200_FPPC_FSCLK_DELAY_MASK 0x07000000
#	define MQ200_FPPC_FSCLK_DELAY_SHIFT 24
	/* bits 31-27 are reserved */

/* FP General Purpose Output Port Control	(index: 08h)	*/
#define MQ200_FPGPOCR		(MQ200_FP + 0x08)
#	define MQ200_FPGPOC_ENCTL_EN	(0<<0)
#	define MQ200_FPGPOC_GPO0_EN	(1<<0)
#	define MQ200_FPGPOC_OSCCLK_EN	(2<<0)
#	define MQ200_FPGPOC_PLL3_EN	(3<<0)
#	define MQ200_FPGPOC_ENVEE_EN	(0<<2)
#	define MQ200_FPGPOC_GPO1_EN	(1<<2)
#	define MQ200_FPGPOC_PWM0_EN	(0<<4)
#	define MQ200_FPGPOC_GPO2_EN	(1<<4)
#	define MQ200_FPGPOC_PWM1_EN	(0<<6)
#	define MQ200_FPGPOC_GPO3_EN	(1<<6)
#	define MQ200_FPGPOC_ENVDD_EN	(0<<8)
#	define MQ200_FPGPOC_GPO4_EN	(1<<9)
#	define MQ200_FPGPOC_PWM_MAX	(0<<10)
#	define MQ200_FPGPOC_PWM_MID	(1<<10)
#	define MQ200_FPGPOC_PWM_MID2	(2<<10)
#	define MQ200_FPGPOC_PWM_MIN	(3<<10)
#	define MQ200_FPGPOC_GPO_MAX	(0<<12)
#	define MQ200_FPGPOC_GPO_MID	(1<<12)
#	define MQ200_FPGPOC_GPO_MID2	(2<<12)
#	define MQ200_FPGPOC_GPO_MIN	(3<<12)
#	define MQ200_FPGPOC_DRV_MAX	(0<<14)
#	define MQ200_FPGPOC_DRV_MID	(1<<14)
#	define MQ200_FPGPOC_DRV_MID2	(2<<14)
#	define MQ200_FPGPOC_DRV_MIN	(3<<14)
#	define MQ200_FPGPOC_GPO0	(1<<16)
#	define MQ200_FPGPOC_GPO1	(1<<17)
#	define MQ200_FPGPOC_GPO2	(1<<18)
#	define MQ200_FPGPOC_GPO3	(1<<19)
#	define MQ200_FPGPOC_GPO4	(1<<20)
	/* bits 31-21 are reserved */

/* FP General Purpose I/O Port Control	(index: 0Ch)	*/
#define MQ200_FPGPOICR		(MQ200_FP + 0x0c)
#	define MQ200_FPGPIOC_INPUT0_EN	(0<<0)
#	define MQ200_FPGPIOC_OUTPUT0_EN	(1<<0
#	define MQ200_FPGPIOC_PLL1_EN	(2<<0)
#	define MQ200_FPGPIOC_CRCBLUE_EN	(3<<0)
#	define MQ200_FPGPIOC_INPUT1_EN	(0<<2)
#	define MQ200_FPGPIOC_OUTPUT1_EN	(1<<2
#	define MQ200_FPGPIOC_PLL2_EN	(2<<2)
#	define MQ200_FPGPIOC_CRCGREEN_EN (3<<2)
#	define MQ200_FPGPIOC_INPUT2_EN	(0<<4)
#	define MQ200_FPGPIOC_OUTPUT2_EN	(1<<4
#	define MQ200_FPGPIOC_PMCLK_EN	(2<<4)
#	define MQ200_FPGPIOC_CRCRED_EN	(3<<4)
	/* bits 15-6 are reserved */
#	define MQ200_FPGPIOC_OUTPUT0	(1<<16)
#	define MQ200_FPGPIOC_OUTPUT1	(1<<17)
#	define MQ200_FPGPIOC_OUTPUT2	(1<<18)
	/* bits 23-19 are reserved */
#	define MQ200_FPGPIOC_INPUT0	(1<<24)
#	define MQ200_FPGPIOC_INPUT1	(1<<25)
#	define MQ200_FPGPIOC_INPUT2	(1<<26)
	/* bits 31-27 are reserved */

/* FP STN Panel Control	(index: 10h)	*/
#define MQ200_FPSTNCR		(MQ200_FP + 0x10)
#	define MQ200_FPSTNC_FRCPRM0_MASK	0x000000ff
#	define MQ200_FPSTNC_FRCPRM0_SHIFT	0
#	define MQ200_FPSTNC_FRCPRM1_MASK	0x0000ff00
#	define MQ200_FPSTNC_FRCPRM1_SHIFT	8
#	define MQ200_FPSTNC_FRCPRM2_MASK	0x00ff0000
#	define MQ200_FPSTNC_FRCPRM2_SHIFT	16
#	define MQ200_FPSTNC_FMOD_MASK		0x7f000000
#	define MQ200_FPSTNC_FMOD_SHIFT		24
#	define MQ200_FPSTNC_FMOD_FRAMECLK	(0<<31)
#	define MQ200_FPSTNC_FMOD_LINECLK	(0<<31)

/* FP D-STN Half-Frame Buffer Control	(index: 14h)	*/
#define MQ200_FPHFBCR		(MQ200_FP + 0x14)
#	define MQ200_FPHFBC_START_MASK	0x00003fff
#	define MQ200_FPHFBC_START_SHIFT	-7	/* XXX, does this work? */
	/* bits 15-14 are reserved */
#	define MQ200_FPHFBC_END_MASK	0xffff0000
#	define MQ200_FPHFBC_END_SHIFT	(16-4)	/* XXX, does this work? */

/* FP Pulse Width Modulation Control	(index: 3Ch)	*/
#define MQ200_FPPWMCR		(MQ200_FP + 0x3c)
#	define MQ200_FPPWMC_PWM0_OSCCLK		(0<<0)
#	define MQ200_FPPWMC_PWM0_BUSCLK		(1<<0)
#	define MQ200_FPPWMC_PWM0_PMCLK		(2<<0)
#	define MQ200_FPPWMC_PWM0_PWSEQ_EN	(0<<2)
#	define MQ200_FPPWMC_PWM0_PWSEQ_DISABLE	(1<<2)
	/* bit 3 is reserved */
#	define MQ200_FPPWMC_PWM0_DIV_MASK	0x000000f0
#	define MQ200_FPPWMC_PWM0_DIV_SHIFT	4
#	define MQ200_FPPWMC_PWM0_DCYCLE_MASK	0x0000ff00
#	define MQ200_FPPWMC_PWM0_DCYCLE_SHIFT	8
#	define MQ200_FPPWMC_PWM1_OSCCLK		(0<<16)
#	define MQ200_FPPWMC_PWM1_BUSCLK		(1<<16)
#	define MQ200_FPPWMC_PWM1_PMCLK		(2<<16)
#	define MQ200_FPPWMC_PWM1_PWSEQ_EN	(0<<18)
#	define MQ200_FPPWMC_PWM1_PWSEQ_DISABLE	(1<<18)
	/* bit 19 is reserved */
#	define MQ200_FPPWMC_PWM1_DIV_MASK	0x00f00000
#	define MQ200_FPPWMC_PWM1_DIV_SHIFT	20
#	define MQ200_FPPWMC_PWM1_DCYCLE_MASK	0xff000000
#	define MQ200_FPPWMC_PWM1_DCYCLE_SHIFT	24

/* FP Frame Rate Control Pattern	(index: 40h to BCh)	*/
#define MQ200_FPFRCPR		(MQ200_FP + 0x40)

/* FP Frame Rate Control Weight	(index: C0h to DCh)	*/
#define MQ200_FPFRCWR		(MQ200_FP + 0xC0)

/*
 * Color Palette 1
 */
#define MQ200_CP(cp, idx)	(MQ200_CP1 + (idx) * 4)	*/
#	define MQ200_GC_BLUE_MASK		0x00ff0000
#	define MQ200_GC_BLUE_SHIFT		16
#	define MQ200_GC_GREEN_MASK		0x0000ff00
#	define MQ200_GC_GREEN_SHIFT		8
#	define MQ200_GC_RED_MASK		0x000000ff
#	define MQ200_GC_RED_SHIFT		0
#	define MQ200_GC_RGB(r, g, b) \
		(((((unsigned long)(r))&0xff)<<0) | \
		    ((((unsigned long)(g))&0xff)<<8) | \
		    ((((unsigned long)(b))&0xff)<<16))

/*
 * Device Configration
 */

/*
 * PCI configuration space
 */
#define MQ200_PC00R		(MQ200_PC+0x00)	/* device/vendor ID	*/
#define MQ200_PC04R		(MQ200_PC+0x04)	/* command/status	*/
#define MQ200_PC08R		(MQ200_PC+0x04)	/* calss code/revision	*/

#define MQ200_PMR		(MQ200_PC+0x40)	/* power management	*/
#define MQ200_PMCSR		(MQ200_PC+0x44)	/* control/status	*/
