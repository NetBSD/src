/*	$NetBSD: mq200reg.h,v 1.1.4.5 2001/03/12 13:28:37 bouyer Exp $	*/

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

#define MQ200_CLOCK_BUS		0
#define MQ200_CLOCK_PLL1	1
#define MQ200_CLOCK_PLL2	2
#define MQ200_CLOCK_PLL3	3

#define MQ200_FRAMEBUFFER	0x000000	/* frame buffer base address */
#define MQ200_PM		0x600000	/* power management	*/
#define MQ200_CC		0x602000	/* CPU interface	*/
#define MQ200_MM		0x604000	/* memory interface unit */
#define MQ200_IN		0x608000	/* interrupt controller	*/
#define MQ200_GC(n)		(0x60a000+0x80*(n))
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
#define MQ200_MMR(n)		(MQ200_MM+(n)*4)
#	define MQ200_MM00_ENABLE		(1<<0)
#	define MQ200_MM00_RESET		(1<<1)
#	define MQ200_MM00_DRAM_RESET	(1<<2)
#	define MQ200_MM01_CLK_PLL1	(0<<0)
#	define MQ200_MM01_CLK_BUS	(1<<0)
#	define MQ200_MM01_CLK_PLL2	(1<<0)

/*
 * Interrupt Controller
 */

/*
 * Graphics Controller 1/2
 */
#define MQ200_GC1		0	/* graphice controller 1*/
#define MQ200_GC2		1	/* graphice controller 2*/
#define MQ200_GCR(n)		(MQ200_GC(0)+(n)*4)
/* GC Control (GC00R and GC20R)	*/
#define MQ200_GCCR(n)		(MQ200_GC(n)+0x00)
#	define MQ200_GCC_ENABLE		(1<<0)
#	define MQ200_GCC_HCRESET	(1<<1)
#	define MQ200_GCC_VCRESET	(1<<2)
#	define MQ200_GCC_WINEN		(1<<3)
#	define MQ200_GCC_DEPTH_SHIFT	4
#	define MQ200_GCC_DEPTH_MASK	0x000000f0
#	define MQ200_GCC_HCEN		(1<<8)
	/* bits 10-9 is reserved */
#	define MQ200_GCC_ALTEN		(1<<11)
#	define MQ200_GCC_ALTDEPTH_SHIFT 12
#	define MQ200_GCC_ALTDEPTH_MASK	0x0000f000
#	define MQ200_GCC_RCLK_SHIFT	16
#	define MQ200_GCC_RCLK_MASK	0x00030000
#	define MQ200_GCC_RCLK_BUS	0x00000000
#	define MQ200_GCC_RCLK_PLL1	0x00010000
#	define MQ200_GCC_RCLK_PLL2	0x00020000
#	define MQ200_GCC_RCLK_PLL3	0x00030000
#	define MQ200_GCC_TESTMODE0	(1<<18)
#	define MQ200_GCC_TESTMODE1	(1<<19)
	/* FD(first clock divisor) is 1, 1.5, 2.5, 3.5, 4.5, 5.6 or 6.5 */
#	define MQ200_GCC_MCLK_FD_SHIFT	20
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

/* GC CRT Control (GC1only)	*/
#define MQ200_GC1CRTCR		MQ200_GCR(0x01)
#	define MQ200_GC1CRTC_DACEN		(1<<0)
#	define MQ200_GC1CRTC_HSYNC_PMCLK	(1<<2)
#	define MQ200_GC1CRTC_VSYNC_PMCLK	(1<<3)
#	define MQ200_GC1CRTC_HSYNC_PMMASK	0x00000030
#	define MQ200_GC1CRTC_HSYNC_PMNORMAL	0x00000000
#	define MQ200_GC1CRTC_HSYNC_PMLOW	0x00000010
#	define MQ200_GC1CRTC_HSYNC_PMHIGH	0x00000020
#	define MQ200_GC1CRTC_VSYNC_PMMASK	0x000000c0
#	define MQ200_GC1CRTC_VSYNC_PMNORMAL	0x00000000
#	define MQ200_GC1CRTC_VSYNC_PMLOW	0x00000040
#	define MQ200_GC1CRTC_VSYNC_PMHIGH	0x00000080
#	define MQ200_GC1CRTC_HSYNC_ACTVHIGH	(0<<8)
#	define MQ200_GC1CRTC_HSYNC_ACTVLOW	(1<<8)
#	define MQ200_GC1CRTC_VSYNC_ACTVHIGH	(0<<9)
#	define MQ200_GC1CRTC_VSYNC_ACTVLOW	(1<<9)
#	define MQ200_GC1CRTC_SYNC_PEDESTAL_EN	(1<<10)
#	define MQ200_GC1CRTC_BLANK_PEDESTAL_EN	(1<<11)
#	define MQ200_GC1CRTC_COMPOSITE_SYNC_EN	(1<<12)
#	define MQ200_GC1CRTC_VREF_INTR		(0<<13)
#	define MQ200_GC1CRTC_VREF_EXTR		(1<<13)
#	define MQ200_GC1CRTC_MONITOR_SENCE_EN	(1<<14)
#	define MQ200_GC1CRTC_CONSTANT_OUTPUT_EN	(1<<15)
#	define MQ200_GC1CRTC_OUTPUT_LEVEL_MASK	0x00ff0000
#	define MQ200_GC1CRTC_OUTPUT_LEVEL_SHIFT	16
#	define MQ200_GC1CRTC_BLUE_NOTLOADED	(1<<24)
#	define MQ200_GC1CRTC_RED_NOTLOADED	(1<<25)
#	define MQ200_GC1CRTC_GREEN_NOTLOADED	(1<<26)
	/* bit 27 is reserved */
#	define MQ200_GC1CRTC_COLOR		(0<<28)
#	define MQ200_GC1CRTC_MONO		(1<<28)
	/* bits 31-29 are reserved */

/* GC CRC Control (GC2 only)	*/
#define MQ200_GC2CRCCR		MQ200_GCR(0x21)
#	define MQ200_GC2CRCC_ENABLE		(1<<0)
#	define MQ200_GC2CRCC_WAIT1VSYNC		(0<<1)
#	define MQ200_GC2CRCC_WAIT2VSYNC		(1<<1)
#	define MQ200_GC2CRCC_BLUE		(0x0<<2)
#	define MQ200_GC2CRCC_GREEN		(0x1<<2)
#	define MQ200_GC2CRCC_RED		(0x2<<2)
#	define MQ200_GC2CRCC_RESULT_SHIFT	8
#	define MQ200_GC2CRCC_RESULT_MASK	0x3fffff00

/* GC Hotizontal Display Control (GC02R and GC22R)	*/
#define MQ200_GCHDCR(n)		(MQ200_GC(n)+0x08)
#	define MQ200_GC1HDC_TOTAL_MASK		0x00000fff
#	define MQ200_GC1HDC_TOTAL_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHDC_END_MASK		0x0fff0000
#	define MQ200_GCHDC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Display Control (GC03R and GC23R)	*/
#define MQ200_GCVDCR(n)		(MQ200_GC(n)+0x0c)
#	define MQ200_GC1VDC_TOTAL_MASK		0x00000fff
#	define MQ200_GC1VDC_TOTAL_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCVDC_END_MASK		0x0fff0000
#	define MQ200_GCVDC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Hotizontal Sync Control (GC04R and GC24R)	*/
#define MQ200_GCHSCR(n)		(MQ200_GC(n)+0x10)
#	define MQ200_GCHSC_START_MASK		0x00000fff
#	define MQ200_GCHSC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHSC_END_MASK		0x0fff0000
#	define MQ200_GCHSC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Sync Control (GC05R and GC25R)	*/
#define MQ200_GCVSCR(n)		(MQ200_GC(n)+0x14)
#	define MQ200_GCVSC_START_MASK		0x00000fff
#	define MQ200_GCVSC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCVSC_END_MASK		0x0fff0000
#	define MQ200_GCVSC_END_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Vertical Display Count (GC07R)	*/
#define MQ200_GC1VDCNTR		MQ200_GCR(0x07)
#	define MQ200_GC1VDCNT_MASK		0x00000fff
	/* bits 31-12 are reserved */

/* GC Window Horizontal Control (GC08R and GC28R)	*/
#define MQ200_GCWHCR(n)		(MQ200_GC(n)+0x20)
#	define MQ200_GCWHC_START_MASK		0x00000fff
#	define MQ200_GCWHC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCWHC_WIDTH_MASK		0x0fff0000
#	define MQ200_GCWHC_WIDTH_SHIFT		16
	/* ALD: Additional Line Delta (GC1 only) */
#	define MQ200_GC1WHC_ALD_MASK		0xf0000000
#	define MQ200_GC1WHC_ALD_SHIFT		28

/* GC Window Vertical Control (GC09R and GC29R)	*/
#define MQ200_GCWVCR(n)		(MQ200_GC(n)+0x24)
#	define MQ200_GCWVC_START_MASK		0x00000fff
#	define MQ200_GCWVC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCWVC_HEIGHT_MASK		0x0fff0000
#	define MQ200_GCWVC_HEIGHT_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Altarnate Window Horizontal Control (GC0AR and GC2AR)	*/
#define MQ200_GCAWHCR(n)	(MQ200_GC(n)+0x28)
#	define MQ200_GCAWHC_START_MASK		0x00000fff
#	define MQ200_GCAWHC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCAWHC_WIDTH_MASK		0x0fff0000
#	define MQ200_GCAWHC_WIDTH_SHIFT		16
	/* ALD: Additional Line Delta (GC1 only) */
#	define MQ200_GC1AWHC_ALD_MASK		0xf0000000
#	define MQ200_GC1AWHC_ALD_SHIFT		28

/* GC Alternate Window Vertical Control (GC0BR and GC2BR)	*/
#define MQ200_GCAWVCR(n)	(MQ200_GC(n)+0x2C)
#	define MQ200_GCAWVC_START_MASK		0x00000fff
#	define MQ200_GCAWVC_START_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCAWVC_HEIGHT_MASK		0x0fff0000
#	define MQ200_GCAWVC_HEIGHT_SHIFT	16
	/* bits 31-28 are reserved */

/* GC Window Start Address (GC0CR and GC2CR)	*/
#define MQ200_GCWSAR(n)		(MQ200_GC(n)+0x30)
#	define MQ200_GCWSA_MASK		0x000fffff
	/* bits 31-21 are reserved */

/* GC Alternate Window Start Address (GC0DR and GC2DR)	*/
#define MQ200_GCAWSAR(n)	(MQ200_GC(n)+0x34)
#	define MQ200_GCAWSA_MASK	0x000fffff
	/* bits 24-21 are reserved */
#	define MQ200_GCAWPI_MASK	0xfe000000
#	define MQ200_GCAWPI_SHIFT	24	/* XXX, 24 could be usefull
						   than 23 */

/* GC Window Stride (GC0ER and GC2ER)	*/
#define MQ200_GCWSTR(n)		(MQ200_GC(n)+0x38)
#	define MQ200_GCWST_MASK		0x0000ffff
#	define MQ200_GCWST_SHIFT	0
#	define MQ200_GCAWST_MASK	0xffff0000
#	define MQ200_GCAWST_SHIFT	16

/* GC2 Line Size (GC2 only, GC2FR)	*/
#define MQ200_GC2LSR		MQ200_GCR(0x2f)
#	define MQ200_GC2WLS_MASK	0x00003fff
#	define MQ200_GC2WLS_SHIFT	0
#	define MQ200_GC2AWLS_MASK	0x3fff0000
#	define MQ200_GC2AWLS_SHIFT	16


/* GC Hardware Cursor Position (GC10R and GC30R)	*/
#define MQ200_GCHCPR(n)		(MQ200_GC(n)+0x40)
#	define MQ200_GCHCP_HSTART_MASK		0x00000fff
#	define MQ200_GCHCP_HSTART_SHIFT		0
	/* bits 15-12 are reserved */
#	define MQ200_GCHCP_VSTART_MASK		0x0fff0000
#	define MQ200_GCHCP_VSTART_SHIFT		16
	/* bits 31-28 are reserved */

/* GC Hardware Start Address and Offset (GC11R and GC31R)	*/
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

/* GC Hardware Cursor Foreground Color (GC13R and GC33R)	*/
#define MQ200_GCHCFCR(n)	(MQ200_GC(n)+0x48)
#	define MQ200_GCHCFC_MASK		0x00ffffff
	/* you can use MQ200_GC_RGB macro	*/
	/* bits 31-24 are reserved */

/* GC Hardware Cursor Background Color (GC14R and GC34R)	*/
#define MQ200_GCHCBCR(n)	(MQ200_GC(n)+0x4c)
#	define MQ200_GCHCBC_MASK		0x00ffffff
	/* you can use MQ200_GC_RGB macro	*/
	/* bits 31-24 are reserved */

#define MQ200_GC1CR		MQ200_GCCR(0)
#define MQ200_GC1HDCR		MQ200_GCHDCR(0)
#define MQ200_GC1VDCR		MQ200_GCVDCR(0)
#define MQ200_GC1HSCR		MQ200_GCHSCR(0)
#define MQ200_GC1VSCR		MQ200_GCVSCR(0)
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
#define MQ200_GC2HDCR		MQ200_GCHDCR(1)
#define MQ200_GC2VDCR		MQ200_GCVDCR(1)
#define MQ200_GC2HSCR		MQ200_GCHSCR(1)
#define MQ200_GC2VSCR		MQ200_GCVSCR(1)
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
#define MQ200_FPR(n)		(MQ200_FP + (n)*4)
/* FP Control	(FP00R)	*/
#define MQ200_FPCR		MQ200_FPR(0)
#	define MQ200_FPC_ENABLE		(1<<0)
#	define MQ200_FPC_GC1		(0<<1)
#	define MQ200_FPC_GC2		(1<<1)
#	define MQ200_FPC_TYPE_MASK	0x000000fc
#	define MQ200_FPC_TYPE_SHIFT	2

#	define MQ200_FPC_TFT		(0<<2)
#	define MQ200_FPC_SSTN		(1<<2)
#	define MQ200_FPC_DSTN		(2<<2)

#	define MQ200_FPC_COLOR		(0<<4)
#	define MQ200_FPC_MONO		(1<<4)

#	define MQ200_FPC_TFTCOLOR	(MQ200_FPC_TFT|MQ200_FPC_COLOR)
#	define MQ200_FPC_SSTNCOLOR	(MQ200_FPC_SSTN|MQ200_FPC_COLOR)
#	define MQ200_FPC_DSTNCOLOR	(MQ200_FPC_DSTN|MQ200_FPC_COLOR)

#	define MQ200_FPC_TFTMONO	(MQ200_FPC_TFT|MQ200_FPC_MONO)
#	define MQ200_FPC_SSTNMONO	(MQ200_FPC_SSTN|MQ200_FPC_MONO)
#	define MQ200_FPC_DSTNMONO	(MQ200_FPC_DSTN|MQ200_FPC_MONO)

#	define MQ200_FPC_TFT4MONO	((0<<5)|MQ200_FPC_TFTMONO)
#	define MQ200_FPC_TFT12		((0<<5)|MQ200_FPC_TFTCOLOR)
#	define MQ200_FPC_SSTN4		((0<<5)|MQ200_FPC_SSTNCOLOR)
#	define MQ200_FPC_DSTN8		((0<<5)|MQ200_FPC_DSTNCOLOR)
#	define MQ200_FPC_TFT6MONO	((1<<5)|MQ200_FPC_TFTMONO)
#	define MQ200_FPC_TFT18		((1<<5)|MQ200_FPC_TFTCOLOR)
#	define MQ200_FPC_SSTN8		((1<<5)|MQ200_FPC_SSTNCOLOR)
#	define MQ200_FPC_DSTN16		((1<<5)|MQ200_FPC_DSTNCOLOR)
#	define MQ200_FPC_TFT8MONO	((2<<5)|MQ200_FPC_TFTMONO)
#	define MQ200_FPC_TFT24		((2<<5)|MQ200_FPC_TFTCOLOR)
#	define MQ200_FPC_SSTN12		((2<<5)|MQ200_FPC_SSTNCOLOR)
#	define MQ200_FPC_DSTN24		((2<<5)|MQ200_FPC_DSTNCOLOR)
#	define MQ200_FPC_SSTN16		((3<<5)|MQ200_FPC_SSTNCOLOR)
#	define MQ200_FPC_SSTN24		((4<<5)|MQ200_FPC_SSTNCOLOR)
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

/* FP Output Pin Control	(FP01R)	*/
#define MQ200_FPPCR		MQ200_FPR(1)
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

/* FP General Purpose Output Port Control	(FP02R)	*/
#define MQ200_FPGPOCR		MQ200_FPR(2)
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

/* FP General Purpose I/O Port Control	(FP03R)	*/
#define MQ200_FPGPOICR		MQ200_FPR(3)
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

/* FP STN Panel Control	(FP04R)	*/
#define MQ200_FPSTNCR		MQ200_FPR(4)
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

/* FP D-STN Half-Frame Buffer Control	(FP05R)	*/
#define MQ200_FPHFBCR		MQ200_FPR(5)
#	define MQ200_FPHFBC_START_MASK	0x00003fff
#	define MQ200_FPHFBC_START_SHIFT	-7	/* XXX, does this work? */
	/* bits 15-14 are reserved */
#	define MQ200_FPHFBC_END_MASK	0xffff0000
#	define MQ200_FPHFBC_END_SHIFT	(16-4)	/* XXX, does this work? */

/* FP Pulse Width Modulation Control	(FP0FR)	*/
#define MQ200_FPPWMCR		MQ200_FPR(0xf)
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

/* FP Frame Rate Control Pattern	(FP10R to FP2FR)	*/
#define MQ200_FPFRCPR(n)	MQ200_FPR(0x10+n)

/* FP Frame Rate Control Weight		(FP30R to FP37R)	*/
#define MQ200_FPFRCWR(n)	MQ200_FPR(0x30+n)

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

/*
 * Power Management
 */
#define MQ200_PMCR	(MQ200_PM + 0x00)
#	define MQ200_PMC_PLL1_N		(1<<0)
#	define MQ200_PMC_PLL2_ENABLE	(1<<2)
#	define MQ200_PMC_PLL3_ENABLE	(1<<3)
#	define MQ200_PMC_IMMEDIATELY	(1<<5)
#	define MQ200_PMC_GE_ENABLE	(1<<8)
#	define MQ200_PMC_GE_FORCE_BUSY	(1<<9)
#	define MQ200_PMC_GE_FORCE_BUSY_LOCAL	(1<<10)
#	define MQ200_PMC_GE_CLK_MASK	0x00001800
#	define MQ200_PMC_GE_CLK_SHIFT	11
#	define MQ200_PMC_GE_CLK_BUS	(0<<11)
#	define MQ200_PMC_GE_CLK_PLL1	(1<<11)
#	define MQ200_PMC_GE_CLK_PLL2	(2<<11)
#	define MQ200_PMC_GE_CLK_PLL3	(3<<11)
#	define MQ200_PMC_GE_COMMAND_RESET	(1<<13)
#	define MQ200_PMC_GE_SOURCE_RESET	(1<<14)
#	define MQ200_PMC_MIU_SEQ_ENABLE	(1<<15)
#	define MQ200_PMC_D3_REFRESH	(1<<16)
#	define MQ200_PMC_D4_REFRESH	(1<<17)
#	define MQ200_PMC_SEQINTVL_MASK	(3<<18)
#	define MQ200_PMC_SEQINTVL_SHIFT	18
#	define MQ200_PMC_SEQINTVL_4		(0<<18)
#	define MQ200_PMC_SEQINTVL_8		(0<<18)
#	define MQ200_PMC_SEQINTVL_16	(0<<18)
#	define MQ200_PMC_SEQINTVL_2048	(0<<18)
#	define MQ200_PMC_FP_SEQINTVL_MASK	(3<<20)
#	define MQ200_PMC_FP_SEQINTVL_SHIFT	20
#	define MQ200_PMC_FP_SEQINTVL_512	(0<<20)
#	define MQ200_PMC_FP_SEQINTVL_1024	(1<<20)
#	define MQ200_PMC_FP_SEQINTVL_2048	(2<<20)
#	define MQ200_PMC_FP_SEQINTVL_128K	(3<<20)
#	define MQ200_PMC_SEQINTVL_ALL	(1<<22)
#	define MQ200_PMC_TESTMODE	(1<<23)
#	define MQ200_PMC_STATE_MASK	(3<<24)
#	define MQ200_PMC_STATE_SHIFT	24
#	define MQ200_PMC_SEQPROGRESS	(1<<26)
#define MQ200_PMD1CR	(MQ200_PM + 0x04)
#define MQ200_PMD2CR	(MQ200_PM + 0x08)

#define MQ200_DCMISCR	(MQ200_DC + 0x00)
#	define MQ200_DCMISC_OSC_BYPASS		(1<<0)
#	define MQ200_DCMISC_OSC_ENABLE		(1<<1)
#	define MQ200_DCMISC_PLL1_BYPASS		(1<<2)
#	define MQ200_DCMISC_PLL1_ENABLE		(1<<3)
#	define MQ200_DCMISC_SA_SLOWBUS		(1<<13)
#	define MQ200_DCMISC_CHIP_RESET		(1<<14)
#	define MQ200_DCMISC_MEMSTANDBY_DISABLE	(1<<15)
#	define MQ200_DCMISC_OSCSHAPER_DISABLE	(1<<24)
#	define MQ200_DCMISC_FASTPOWSEQ_DISABLE	(1<<25)
#	define MQ200_DCMISC_OSCFREQ_MASK	(3<<26)
#	define MQ200_DCMISC_OSCFREQ_12_25	(3<<26)

/*
 * Fout = Fref*(M+1)/(N+1)/(2^P)
 * Fout: PLL output frequency
 * Fref: reference frequency(internal oscillator or external clock)
 */
#define MQ200_PLL1R	(MQ200_DC + 0x00)
#define MQ200_PLL2R	(MQ200_PM + 0x18)
#define MQ200_PLL3R	(MQ200_PM + 0x1c)
#define MQ200_PLL_EXTCLK	(1<<0)
#define MQ200_PLL_BYPASS	(1<<1)
#define MQ200_PLL_P_MASK	0x00000070
#define MQ200_PLL_P_SHIFT	4
#define MQ200_PLL_N_MASK	0x00001f00
#define MQ200_PLL_N_SHIFT	8
#define MQ200_PLL_M_MASK	0x00ff0000
#define MQ200_PLL_M_SHIFT	16
#define MQ200_PLL_PARAM_MASK	(MQ200_PLL_P_MASK|MQ200_PLL_N_MASK|MQ200_PLL_M_MASK)
#define MQ200_PLL_TRIM_MASK	0xf0000000
#define MQ200_PLL_TRIM_SHIFT	28
