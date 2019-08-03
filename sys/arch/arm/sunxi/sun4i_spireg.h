/*	$NetBSD: sun4i_spireg.h,v 1.1 2019/08/03 13:28:42 tnn Exp $	*/

/*
 * Copyright (c) 2019 Tobias Nygren
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SUNXI_SUN4I_SPIREG_H_
#define _SUNXI_SUN4I_SPIREG_H_

#include <sys/cdefs.h>

#define SPI_RXDATA		0x00

#define SPI_TXDATA		0x04

#define SPI_CTL			0x08
#define SPI_CTL_SDM		__BIT(20)
#define SPI_CTL_SDC		__BIT(19)
#define SPI_CTL_TP_EN		__BIT(18)
#define SPI_CTL_SS_LEVEL	__BIT(17)
#define SPI_CTL_SS_CTRL		__BIT(16)
#define SPI_CTL_DHB		__BIT(15)
#define SPI_CTL_DDB		__BIT(14)
#define SPI_CTL_SS		__BITS(13, 12)
#define SPI_CTL_RPSM		__BIT(11)
#define SPI_CTL_XCH		__BIT(10)
#define SPI_CTL_RF_RST		__BIT(9)
#define SPI_CTL_TF_RST		__BIT(8)
#define SPI_CTL_SSCTL		__BIT(7)
#define SPI_CTL_LMTF		__BIT(6)
#define SPI_CTL_DMAMC		__BIT(5)
#define SPI_CTL_SSPOL		__BIT(4)
#define SPI_CTL_POL		__BIT(3)
#define SPI_CTL_PHA		__BIT(2)
#define SPI_CTL_MODE		__BIT(1)
#define SPI_CTL_EN		__BIT(0)

#define SPI_INTCTL			0x0c
#define SPI_INTCTL_SS_INT_EN		__BIT(17)
#define SPI_INTCTL_TX_INT_EN		__BIT(16)
#define SPI_INTCTL_TF_UR_INT_EN		__BIT(14)
#define SPI_INTCTL_TF_OF_INT_EN		__BIT(13)
#define SPI_INTCTL_TF_E34_INT_EN	__BIT(12)
#define SPI_INTCTL_TF_E14_INT_EN	__BIT(11)
#define SPI_INTCTL_TF_FL_INT_EN		__BIT(10)
#define SPI_INTCTL_TF_HALF_EMP_INT_EN	__BIT(9)
#define SPI_INTCTL_TF_EMP_INT_EN	__BIT(8)
#define SPI_INTCTL_RF_UR_INT_EN		__BIT(6)
#define SPI_INTCTL_RF_OF_INT_EN		__BIT(5)
#define SPI_INTCTL_RF_F34_INT_EN	__BIT(4)
#define SPI_INTCTL_RF_F14_INT_EN	__BIT(3)
#define SPI_INTCTL_RF_FU_INT_EN		__BIT(2)
#define SPI_INTCTL_RF_HALF_FU_INT_EN	__BIT(1)
#define SPI_INTCTL_RF_RDY_INT_EN	__BIT(0)

#define SPI_INT_STA		0x10
#define SPI_INT_STA_INT_CBF	__BIT(31)
#define SPI_INT_STA_SSI		__BIT(17)
#define SPI_INT_STA_TC		__BIT(16)
#define SPI_INT_STA_TU		__BIT(14)
#define SPI_INT_STA_TO		__BIT(13)
#define SPI_INT_STA_TE34	__BIT(12)
#define SPI_INT_STA_TE14	__BIT(11)
#define SPI_INT_STA_TF		__BIT(10)
#define SPI_INT_STA_THE		__BIT(9)
#define SPI_INT_STA_TE		__BIT(8)
#define SPI_INT_STA_RU		__BIT(6)
#define SPI_INT_STA_RO		__BIT(5)
#define SPI_INT_STA_RF34	__BIT(4)
#define SPI_INT_STA_RF14	__BIT(3)
#define SPI_INT_STA_RF		__BIT(2)
#define SPI_INT_STA_RHF		__BIT(1)
#define SPI_INT_STA_RR		__BIT(0)

#define SPI_DMACTL		0x14
#define SPI_DMACTL_TF_EMP34_DMA	__BIT(12)
#define SPI_DMACTL_TF_EMP14_DMA	__BIT(11)
#define SPI_DMACTL_TF_NF_DMA	__BIT(10)
#define SPI_DMACTL_TF_HE_DMA	__BIT(9)
#define SPI_DMACTL_TF_EMP_DMA	__BIT(8)
#define SPI_DMACTL_RF_FU34_DMA	__BIT(4)
#define SPI_DMACTL_RF_FU14_DMA	__BIT(3)
#define SPI_DMACTL_RF_FU_DMA	__BIT(2)
#define SPI_DMACTL_RF_HF_DMA	__BIT(1)
#define SPI_DMACTL_RF_RDY_DMA	__BIT(0)

#define SPI_WAIT		0x18
#define SPI_WAIT_WCC		__BITS(15, 0)

#define SPI_CCTL		0x1c
#define SPI_CCTL_DRS		__BIT(12)
#define SPI_CCTL_CDR1		__BITS(11, 8)
#define SPI_CCTL_CDR2		__BITS(7, 0)

#define SPI_BC			0x20
#define SPI_BC_BC		__BITS(23, 0)
	
#define SPI_TC			0x24
#define SPI_TC_WTC		__BITS(23, 0)

#define SPI_FIFO_STA		0x28
#define SPI_FIFO_STA_TF_CNT	__BITS(22, 16)
#define SPI_FIFO_STA_RF_CNT	__BITS(6, 0)

#endif /* _SUNXI_SUN4I_SPIREG_H_ */
