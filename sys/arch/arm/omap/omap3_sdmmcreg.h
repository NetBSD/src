/*	$NetBSD: omap3_sdmmcreg.h,v 1.1.2.5 2017/12/03 11:35:55 jdolecek Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_OMAP_OMAP3_SDMMCREG_H_
#define _ARM_OMAP_OMAP3_SDMMCREG_H_

#define SDMMC1_BASE_3430	0x4809C000
#define SDMMC2_BASE_3430	0x480B4000
#define SDMMC3_BASE_3430	0x480AD000

#define SDMMC1_BASE_3530	0x4809C000
#define SDMMC2_BASE_3530	0x480B4000
#define SDMMC3_BASE_3530	0x480AD000

#define SDMMC1_BASE_4430	0x4809C000	// same for omap5
#define SDMMC2_BASE_4430	0x480B4000	// same for omap5
#define SDMMC3_BASE_4430	0x480AD000	// same for omap5
#define SDMMC4_BASE_4430	0x480D1000	// same for omap5
#define SDMMC5_BASE_4430	0x480D5000	// same for omap5

#define	SDMMC1_BASE_TIAM335X	0x48060000
#define	SDMMC2_BASE_TIAM335X	0x481d8000
#define	SDMMC3_BASE_TIAM335X	0x47810000

#define	OMAP3_SDMMC_SDHC_OFFSET	0x100
#define	OMAP3_SDMMC_SDHC_SIZE	0x100

#define	OMAP4_SDMMC_HL_SIZE	0x100

#define MMCHS_HL_REV		0x000
#define MMCHS_HL_HWINFO		0x004
#  define HL_HWINFO_RETMODE		(1 << 8)
#  define HL_HWINFO_MEM_SIZE_MASK	(0xf << 2)
#  define HL_HWINFO_MEM_SIZE_512	(0x1 << 2)
#  define HL_HWINFO_MEM_SIZE_1024	(0x2 << 2)
#  define HL_HWINFO_MEM_SIZE_2048	(0x4 << 2)
#  define HL_HWINFO_MEM_SIZE_4096	(0x8 << 2)
#  define HL_HWINFO_MEMRGE_MEM		(1 << 1)
#  define HL_HWINFO_MADMA_EN		(1 << 0)
#define MMCHS_HL_SYSCONFIG	0x010
#  define HL_SYSCONFIG_STANDBYMODE_MASK	(0x3 << 2)
#  define HL_SYSCONFIG_STANDBYMODE_FORCE (0 << 2)
#  define HL_SYSCONFIG_STANDBYMODE_NO	(1 << 2)
#  define HL_SYSCONFIG_STANDBYMODE_SMART (2 << 2)
#  define HL_SYSCONFIG_STANDBYMODE_WC	(3 << 2) /* Smart-Idle wakeup-capable */
#  define HL_SYSCONFIG_IDLEMODE_MASK	(0x3 << 2)
#  define HL_SYSCONFIG_IDLEMODE_FORCE	(0 << 2)
#  define HL_SYSCONFIG_IDLEMODE_NO	(1 << 2)
#  define HL_SYSCONFIG_IDLEMODE_SMART	(2 << 2)
#  define HL_SYSCONFIG_IDLEMODE_WC	(3 << 2) /* Smart-Idle wakeup-capable */
#  define HL_SYSCONFIG_FREEEMU		(1 << 1)
#  define HL_SYSCONFIG_SOFTRESET	(1 << 0)

#define MMCHS_SYSCONFIG		0x010	/* System Configuration */
#  define SYSCONFIG_CLOCKACTIVITY_MASK	(3 << 8)
#  define SYSCONFIG_CLOCKACTIVITY_FCLK	(2 << 8)
#  define SYSCONFIG_CLOCKACTIVITY_ICLK	(1 << 8)
#  define SYSCONFIG_SIDLEMODE_MASK	(3 << 3)
#  define SYSCONFIG_SIDLEMODE_AUTO	(2 << 3)
#  define SYSCONFIG_ENAWAKEUP		(1 << 2)
#  define SYSCONFIG_SOFTRESET		(1 << 1)
#  define SYSCONFIG_AUTOIDLE		(1 << 0)
#define MMCHS_SYSSTATUS		0x014	/* System Status */
#  define SYSSTATUS_RESETDONE		(1 << 0)
#define MMCHS_CSRE		0x024	/* Card status response error */
#define MMCHS_SYSTEST		0x028	/* System Test */
#define MMCHS_CON		0x02c	/* Configuration */
#  define CON_SDMA_LNE			(1 << 21)	/*Slave DMA Lvl/Edg Rq*/
#  define CON_MNS			(1 << 20)	/* DMA Mstr/Slv sel */
#  define CON_DDR			(1 << 19)	/* Dual Data Rate */
#  define CON_CF0			(1 << 18)	/*Boot status support*/
#  define CON_BOOTACK			(1 << 17)	/*Boot acknowledge rcv*/
#  define CON_CLKEXTFREE		(1 << 16)
#  define CON_PADEN			(1 << 15)	/* Ctrl Pow for MMC */
#  define CON_OBIE			(1 << 14)	/* Out-of-Band Intr */
#  define CON_OBIP			(1 << 13)	/*O-of-B Intr Polarity*/
#  define CON_CEATA			(1 << 12)	/* CE-ATA */
#  define CON_CTPL			(1 << 11)	/* Ctrl Power dat[1] */
#  define CON_DVAL_33US			(0 << 9)	/* debounce filter val*/
#  define CON_DVAL_231US		(1 << 9)	/* debounce filter val*/
#  define CON_DVAL_1MS			(2 << 9)	/* debounce filter val*/
#  define CON_DVAL_8_4MS		(3 << 9)	/*   8.4ms */
#  define CON_WPP			(1 << 8)	/* Write protect pol */
#  define CON_CDP			(1 << 7)	/*Card detect polarity*/
#  define CON_MIT			(1 << 6)	/* MMC interrupt cmd */
#  define CON_DW8			(1 << 5)	/* 8-bit mode */
#  define CON_MODE			(1 << 4)	/* SYSTEST mode */
#  define CON_STR			(1 << 3)	/* Stream command */
#  define CON_HR			(1 << 2)	/* Broadcast host rsp */
#  define CON_INIT			(1 << 1)	/* Send init stream */
#  define CON_OD			(1 << 0)	/* Card open drain */
#define MMCHS_PWCNT		0x030	/* Power counter */

#endif
