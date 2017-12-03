/* $NetBSD: tegra_pmcreg.h,v 1.5.16.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#ifndef _ARM_TEGRA_PMCREG_H
#define _ARM_TEGRA_PMCREG_H

#define PMC_CNTRL_0_REG			0x00

#define PMC_CNTRL_0_CPUPWRGOOD_SEL	__BITS(21,20)
#define PMC_CNTRL_0_CPUPWRGOOD_EN	__BIT(19)
#define PMC_CNTRL_0_FUSE_OVERRIDE	__BIT(18)
#define PMC_CNTRL_0_INTR_POLARITY	__BIT(17)
#define PMC_CNTRL_0_CPUPWRREG_OE	__BIT(16)
#define PMC_CNTRL_0_CPUPWRREG_POLARITY	__BIT(15)
#define PMC_CNTRL_0_SIDE_EFFECT_LP0	__BIT(14)
#define PMC_CNTRL_0_AOINIT		__BIT(13)
#define PMC_CNTRL_0_PWRGATE_DIS		__BIT(12)
#define PMC_CNTRL_0_SYSCLK_OE		__BIT(11)
#define PMC_CNTRL_0_SYSCLK_POLARITY	__BIT(10)
#define PMC_CNTRL_0_PWRREQ_OE		__BIT(9)
#define PMC_CNTRL_0_PWRREQ_POLARITY	__BIT(8)
#define PMC_CNTRL_0_BLINK_EN		__BIT(7)
#define PMC_CNTRL_0_GLITCHDET_DIS	__BIT(6)
#define PMC_CNTRL_0_LATCHWAKE_EN	__BIT(5)
#define PMC_CNTRL_0_MAIN_RST		__BIT(4)
#define PMC_CNTRL_0_KBC_RST		__BIT(3)
#define PMC_CNTRL_0_RTC_RST		__BIT(2)
#define PMC_CNTRL_0_RTC_CLK_DIS		__BIT(1)
#define PMC_CNTRL_0_KBC_CLK_DIS		__BIT(0)

#define PMC_PWRGATE_TOGGLE_0_REG	0x30

#define PMC_PWRGATE_TOGGLE_0_START	__BIT(8)
#define PMC_PWRGATE_TOGGLE_0_PARTID	__BITS(4,0)

#define PMC_REMOVE_CLAMPING_CMD_0_REG	0x34

#define PMC_PWRGATE_STATUS_0_REG	0x38

#define PMC_PARTID_IRAM			24
#define PMC_PARTID_VIC			23
#define PMC_PARTID_XUSBC		22
#define PMC_PARTID_XUSBB		21
#define PMC_PARTID_XUSBA		20
#define PMC_PARTID_DISB			19
#define PMC_PARTID_DIS			18
#define PMC_PARTID_SOR			17
#define PMC_PARTID_C1NC			16
#define PMC_PARTID_C0NC			15
#define PMC_PARTID_CE0			14
#define PMC_PARTID_A9LP			12
#define PMC_PARTID_CPU3			11
#define PMC_PARTID_CPU2			10
#define PMC_PARTID_CPU1			9
#define PMC_PARTID_SAX			8
#define PMC_PARTID_HEG			7
#define PMC_PARTID_MPE			6
#define PMC_PARTID_L2C			5
#define PMC_PARTID_VDE			4
#define PMC_PARTID_PCX			3
#define PMC_PARTID_VE			2
#define PMC_PARTID_TD			1
#define PMC_PARTID_CPU0			0

#define PMC_IO_DPD_STATUS_REG		0x1bc
#define PMC_IO_DPD_STATUS_HDMI		__BIT(28)

#define PMC_IO_DPD2_STATUS_REG		0x1c4
#define PMC_IO_DPD2_STATUS_HV		__BIT(6)

#define PMC_GPU_RG_CNTRL_REG		0x2d4
#define PMC_GPU_RG_CNTRL_RAIL_CLAMP	__BIT(0)

#endif /* _ARM_TEGRA_PMCREG_H */
