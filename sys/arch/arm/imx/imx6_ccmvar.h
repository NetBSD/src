/*	$NetBSD: imx6_ccmvar.h,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/
/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_IMX_IMX6_CCMVAR_H_
#define	_ARM_IMX_IMX6_CCMVAR_H_

enum imx6_clock {
	IMX6CLK_PLL1,		/* = PLL_ARM */
	IMX6CLK_PLL2,		/* = PLL_SYS = 528_PLL (24MHz * 22) */
	IMX6CLK_PLL3,		/* = PLL_USB1 = 480_PLL1 */
				/*  (USB/OTG PHY, 480PFD0-480PFD3, 24MHz*20) */
	IMX6CLK_PLL4,		/* = PLL_AUDIO */
	IMX6CLK_PLL5,		/* = PLL_VIDEO */
	IMX6CLK_PLL6,		/* = PLL_ENET (20MHz = 24MHz * 5/6) */
	IMX6CLK_PLL7,		/* = PLL_USB2 (USB2 PHY, HOST PHY, 24MHz*20) */
	IMX6CLK_PLL8,		/* = PLL_MLB (Media Link Bus) */
	IMX6CLK_PLL2_PFD0,
	IMX6CLK_PLL2_PFD1,
	IMX6CLK_PLL2_PFD2,
	IMX6CLK_PLL3_PFD0,
	IMX6CLK_PLL3_PFD1,
	IMX6CLK_PLL3_PFD2,
	IMX6CLK_PLL3_PFD3,

	IMX6CLK_ARM_ROOT,	/* CPU clock of ARM core */
	IMX6CLK_PERIPH,
	IMX6CLK_AHB,
	IMX6CLK_IPG,
	IMX6CLK_AXI,
	IMX6CLK_MMDC_CH0,
	IMX6CLK_MMDC_CH1,

	IMX6CLK_USDHC1_CLK_ROOT,
	IMX6CLK_USDHC2_CLK_ROOT,
	IMX6CLK_USDHC3_CLK_ROOT,
	IMX6CLK_USDHC4_CLK_ROOT,
};

uint32_t imx6_get_clock(enum imx6_clock);
int imx6_set_clock(enum imx6_clock, uint32_t);
int imx6_pll_power(uint32_t, int);

uint32_t imx6_ccm_read(uint32_t);
void imx6_ccm_write(uint32_t, uint32_t);

#endif	/* _ARM_IMX_IMX6_CCMVAR_H_ */
