/*	$NetBSD: imx6_ahcisatareg.h,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX6_AHCISATAREG_H_
#define _ARM_IMX_IMX6_AHCISATAREG_H_

#define SATA_CAP				0x00000000
#define  SATA_CAP_SSS				__BIT(27)
#define SATA_PI					0x0000000c
#define  SATA_PI_PI				__BIT(0)
#define SATA_BISTAFR				0x000000a0
#define SATA_BISTCR				0x000000a4
#define SATA_BISTFCTR				0x000000a8
#define SATA_BISTSR				0x000000ac
#define SATA_OOBR				0x000000bc
#define SATA_GPCR				0x000000d0
#define SATA_GPSR				0x000000d4
#define SATA_TIMER1MS				0x000000e0
#define SATA_TESTR				0x000000f4
#define SATA_VERSIONR				0x000000f8

#define SATA_P0DMACR				0x00000170
#define  SATA_P0DMACR_RXTS(n)			__SHIFTIN(n, __BITS(7, 4))
#define  SATA_P0DMACR_TXTS(n)			__SHIFTIN(n, __BITS(3, 0))
#define SATA_P0PHYCR				0x00000178
#define  SATA_P0PHYCR_CR_READ			__BIT(19)
#define  SATA_P0PHYCR_CR_WRITE			__BIT(18)
#define  SATA_P0PHYCR_CR_CAP_DATA		__BIT(17)
#define  SATA_P0PHYCR_CR_CAP_ADDR		__BIT(16)
#define  SATA_P0PHYCR_CR_DATA_IN(v)		((v) & 0xffff)
#define SATA_P0PHYSR				0x0000017c
#define  SATA_P0PHYSR_CR_ACK			__BIT(18)
#define  SATA_P0PHYSR_CR_DATA_OUT(v)		((v) & 0xffff)

/* phy registers */
#define SATA_PHY_CLOCK_CTL_OVRD			0x0013
#define SATA_PHY_CLOCK_CTL_OVRD_MPLL_PWRON	__BIT(2)

#define SATA_PHY_CLOCK_RESET			0x7f3f
#define SATA_PHY_CLOCK_RESET_RST		__BIT(0)

#define SATA_PHY_LANE0_OUT_STAT			0x2003
#define SATA_PHY_LANE0_OUT_STAT_RX_PLL_STATE	__BIT(1)

#define SATA_PHY_LANE0_TX_OVRD			0x2004
#define SATA_PHY_LANE0_TX_OVRD_TX_EN(n)		__SHIFTIN(n, __BITS(3, 1))
#define SATA_PHY_LANE0_RX_OVRD			0x2005
#define SATA_PHY_LANE0_RX_OVRD_RX_EN		__BIT(2)
#define SATA_PHY_LANE0_RX_OVRD_RX_PLL_PWRON	__BIT(1)

#endif /* _ARM_IMX_IMX6_AHCISATAREG_H_ */
