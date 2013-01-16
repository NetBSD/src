/*	$NetBSD: omap3_sdmmcreg.h,v 1.2.2.3 2013/01/16 05:32:49 yamt Exp $	*/
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

#define	OMAP3_SDMMC_SDHC_OFFSET	0x100
#define	OMAP3_SDMMC_SDHC_SIZE	0x100

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
#define MMCHS_SYSCTL		0x12c	/* SD system control register */
#  define SYSCTL_CEN			(1 << 2)	/* Clock enable */

#endif
