/* $NetBSD: tegra_pciereg.h,v 1.4 2017/09/26 16:12:45 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_TEGRA_PCIEREG_H
#define _ARM_TEGRA_PCIEREG_H

/* PADS */
#define PADS_REFCLK_CFG0_REG	0xc8

/* AFI */
#define AFI_AXI_NBAR		9

#define AFI_AXI_BARi_SZ(i)	((i) < 6 ? \
    0x000 + ((i) - 0) * 0x04 : \
    0x134 + ((i) - 6) * 0x04)

#define AFI_AXI_BARi_START(i)	((i) < 6 ? \
    0x018 + ((i) - 0) * 0x04 : \
    0x140 + ((i) - 6) * 0x04)

#define AFI_FPCI_BARi(i)	((i) < 6 ? \
    0x030 + ((i) - 0) * 0x04 : \
    0x14c + ((i) - 6) * 0x04)

#define AFI_MSI_BAR_SZ_REG	0x60
#define AFI_MSI_FPCI_BAR_ST_REG	0x64
#define AFI_MSI_AXI_BAR_ST_REG	0x68
#define AFI_INTR_MASK_REG	0xb4
#define AFI_INTR_CODE_REG	0xb8
#define AFI_INTR_SIGNATURE_REG	0xbc
#define AFI_SM_INTR_ENABLE_REG	0xc4
#define AFI_AFI_INTR_ENABLE_REG	0xc8
#define AFI_PCIE_CONFIG_REG	0xf8
#define AFI_PEXn_CTRL_REG(n)	(0x110 + (n) * 8)
#define AFI_PEXn_STATUS_REG(n)	(0x114 + (n) * 8)
#define AFI_PLLE_CONTROL_REG	0x160
#define AFI_PEXBIAS_CTRL_REG	0x168
#define AFI_MSG_REG		0x190

#define AFI_INTR_MASK_MSI	__BIT(8)
#define AFI_INTR_MASK_INT	__BIT(0)

#define AFI_INTR_CODE_INT_CODE	__BITS(4,0)
#define AFI_INTR_CODE_SM_MSG	6

#define AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG		__BITS(23,20)
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_2_1	0
#define  AFI_PCIE_CONFIG_SM2TMS0_XBAR_CONFIG_4_1	1
#define  AFI_PCIE_CONFIG_PCIECn_DISABLE_DEVICE(n)	__BIT(1 + (n))

#define AFI_PEXn_CTRL_REFCLK_OVERRIDE_EN		__BIT(4)
#define AFI_PEXn_CTRL_REFCLK_EN				__BIT(3)
#define AFI_PEXn_CTRL_CLKREQ_EN				__BIT(1)
#define AFI_PEXn_CTRL_RST_L				__BIT(0)

#define AFI_PLLE_CONTROL_BYPASS_PADS2PLLE_CONTROL	__BIT(9)
#define AFI_PLLE_CONTROL_PADS2PLLE_CONTROL_EN		__BIT(1)

#define AFI_PEXBIAS_CTRL_PWRD	__BIT(0)

#define AFI_MSG_INT1		__BITS(27,24)
#define AFI_MSG_PM_PME1		__BIT(20)
#define AFI_MSG_INT0		__BITS(11,8)
#define AFI_MSG_PM_PME0		__BIT(4)

#endif /* _ARM_TEGRA_PCIEREG_H */
