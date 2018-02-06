/*	$NetBSD: sun6i_spireg.h,v 1.1 2018/02/06 12:45:39 jakllsch Exp $	*/

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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

#ifndef _SUNXI_SUN6I_SPIREG_H_
#define _SUNXI_SUN6I_SPIREG_H_

#include <sys/cdefs.h>

#define SPI_GCR		0x004
#define SPI_GCR_SRST		__BIT(31)
#define SPI_GCR_TP_EN		__BIT(7)
#define SPI_GCR_MODE		__BIT(1)
#define SPI_GCR_MODE_SLAVE	(0)
#define SPI_GCR_MODE_MASTER	(1)
#define SPI_GCR_EN		__BIT(0)

#define SPI_TCR		0x008
#define SPI_TCR_XCH		__BIT(31)
#define SPI_TCR_SDDM		__BIT(14)
#define SPI_TCR_SDM		__BIT(13)
#define SPI_TCR_FBS		__BIT(12)
#define SPI_TCR_SDC		__BIT(11)
#define SPI_TCR_RPSM		__BIT(10)
#define SPI_TCR_DDB		__BIT(9)
#define SPI_TCR_DHB		__BIT(8)
#define SPI_TCR_SS_LEVEL	__BIT(7)
#define SPI_TCR_SS_OWNER	__BIT(6)
#define SPI_TCR_SS_SEL		__BITS(5,4)
#define SPI_TCR_SSCTL		__BIT(3)
#define SPI_TCR_SPOL		__BIT(2)
#define SPI_TCR_CPOL		__BIT(1)
#define SPI_TCR_CPHA		__BIT(0)

#define SPI_IER		0x010
#define SPI_IER_SS_INT_EN	__BIT(13)
#define SPI_IER_TC_INT_EN	__BIT(12)
#define SPI_IER_TF_UDR_INT_EN	__BIT(11)
#define SPI_IER_TF_OVF_INT_EN	__BIT(10)
#define SPI_IER_RF_UDR_INT_EN	__BIT(9)
#define SPI_IER_RF_OVF_INT_EN	__BIT(8)
#define SPI_IER_TF_FUL_INT_EN	__BIT(6)
#define SPI_IER_TX_EMP_INT_EN	__BIT(5)
#define SPI_IER_TX_ERQ_INT_EN	__BIT(4)
#define SPI_IER_RF_FUL_INT_EN	__BIT(2)
#define SPI_IER_RX_EMP_INT_EN	__BIT(1)
#define SPI_IER_RF_RDY_INT_EN	__BIT(0)

#define SPI_INT_STA	0x014
#define SPI_ISR_SSI		__BIT(13)
#define SPI_ISR_TC		__BIT(12)
#define SPI_ISR_TF_UDF		__BIT(11)
#define SPI_ISR_TF_OVF		__BIT(10)
#define SPI_ISR_RX_UDF		__BIT(9)
#define SPI_ISR_RX_OVF		__BIT(8)
#define SPI_ISR_TX_FULL		__BIT(6)
#define SPI_ISR_TX_EMP		__BIT(5)
#define SPI_ISR_TX_READY	__BIT(4)
#define SPI_ISR_RX_FULL		__BIT(2)
#define SPI_ISR_RX_EMP		__BIT(1)
#define SPI_ISR_RX_RDY		__BIT(0)

#define SPI_FCR		0x018
#define SPI_FCR_TX_FIFO_RST	__BIT(31)
#define SPI_FCR_TF_TEST		__BIT(30)
#define SPI_FCR_TF_DRQ_EN	__BIT(24)
#define SPI_FCR_TX_TRIG_LEVEL	__BITS(23,16)
#define SPI_FCR_RF_RST		__BIT(15)
#define SPI_FCR_RF_TEST		__BIT(14)
#define SPI_FCR_RX_DMA_MODE	__BIT(9)
#define SPI_FCR_RF_DRQ_EN	__BIT(8)
#define SPI_FCR_RX_TRIG_LEVEL	__BITS(7,0)

#define SPI_FSR		0x01c
#define SPI_FSR_TB_WR		__BIT(31)
#define SPI_FSR_TB_CNT		__BITS(30,28)
#define SPI_FSR_TF_CNT		__BITS(23,16)
#define SPI_FSR_RB_WR		__BIT(15)
#define SPI_FSR_RB_CNT		__BITS(14,12)
#define SPI_FSR_RF_CNT		__BITS(7,0)

#define SPI_WCR		0x020
#define SPI_WCR_SWC		__BITS(19,16)
#define SPI_WCR_WCC		__BITS(15,0)

#define SPI_CCTL	0x024
#define SPI_CCTL_DRS		__BIT(12)
#define SPI_CCTL_CDR1		__BITS(11,8)
#define SPI_CCTL_CDR2		__BITS(7,0)

#define SPI_BC		0x030
#define SPI_BC_MBC		__BITS(23,0)

#define SPI_TC		0x034
#define SPI_TC_MWTC		__BITS(23,0)

#define SPI_BCC		0x038
#define SPI_BCC_DRM		__BIT(28)
#define SPI_BCC_DBC		__BITS(27,24)
#define SPI_BCC_STC		__BITS(23,0)

#define SPI_NDMA_CTL	0x088
#define SPI_NDMA_CTL_NDMA_MODE_CTL	__BITS(7,0)

#define SPI_TXD		0x200
#define SPI_TXD_TDATA_4		__BITS(31,0)

#define SPI_RXD		0x300
#define SPI_RXD_RDATA_4		__BITS(31,0)

#endif /* _SUNXI_SUN6I_SPIREG_H_ */
