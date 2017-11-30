/*	$NetBSD: stih416-clks.h,v 1.1.1.2 2017/11/30 19:40:51 jmcneill Exp $	*/

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides constants clk index STMicroelectronics
 * STiH416 SoC.
 */
#ifndef _CLK_STIH416
#define _CLK_STIH416

/* CLOCKGEN A0 */
#define CLK_ICN_REG		0
#define CLK_ETH1_PHY		4

/* CLOCKGEN A1 */
#define CLK_ICN_IF_2		0
#define CLK_GMAC0_PHY		3

#endif
