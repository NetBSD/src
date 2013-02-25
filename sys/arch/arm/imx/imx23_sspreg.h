/* $Id: imx23_sspreg.h,v 1.1.6.2 2013/02/25 00:28:28 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_SSPREG_H_
#define _ARM_IMX_IMX23_SSSREG_H_

#include <sys/cdefs.h>

#define HW_SSP1_BASE 0x80010000
#define HW_SSP1_SIZE 0x2000

#define HW_SSP2_BASE 0x80034000
#define HW_SSP2_SIZE 0x2000

/*
 * SSP Control Register 0.
 */
#define HW_SSP_CTRL0		0x000
#define HW_SSP_CTRL0_SET	0x004
#define HW_SSP_CTRL0_CLR	0x008
#define HW_SSP_CTRL0_TOG	0x00C

#define HW_SSP_CTRL0_SFTRST		__BIT(31)
#define HW_SSP_CTRL0_CLKGATE		__BIT(30)
#define HW_SSP_CTRL0_RUN		__BIT(29)
#define HW_SSP_CTRL0_SDIO_IRQ_CHECK	__BIT(28)
#define HW_SSP_CTRL0_LOCK_CS		__BIT(27)
#define HW_SSP_CTRL0_IGNORE_CRC		__BIT(26)
#define HW_SSP_CTRL0_READ		__BIT(25)
#define HW_SSP_CTRL0_DATA_XFER		__BIT(24)
#define HW_SSP_CTRL0_BUS_WIDTH		__BITS(23, 22)
#define HW_SSP_CTRL0_WAIT_FOR_IRQ	__BIT(21)
#define HW_SSP_CTRL0_WAIT_FOR_CMD	__BIT(20)
#define HW_SSP_CTRL0_LONG_RESP		__BIT(19)
#define HW_SSP_CTRL0_CHECK_RESP		__BIT(18)
#define HW_SSP_CTRL0_GET_RESP		__BIT(17)
#define HW_SSP_CTRL0_ENABLE		__BIT(16)
#define HW_SSP_CTRL0_XFER_COUNT		__BITS(15, 0)

/*
 * SD/MMC Command Register 0.
 */
#define HW_SSP_CMD0	0x010
#define HW_SSP_CMD0_SET	0x014
#define HW_SSP_CMD0_CLR	0x018
#define HW_SSP_CMD0_TOG	0x01C

#define HW_SSP_CMD0_RSVD0		__BITS(31, 23)
#define HW_SSP_CMD0_SLOW_CLKING_EN	__BIT(22)
#define HW_SSP_CMD0_CONT_CLKING_EN	__BIT(21)
#define HW_SSP_CMD0_APPEND_8CYC		__BIT(20)
#define HW_SSP_CMD0_BLOCK_SIZE		__BITS(19, 16)
#define HW_SSP_CMD0_BLOCK_COUNT		__BITS(15, 8)
#define HW_SSP_CMD0_CMD			__BITS(7, 0)

/*
 * SD/MMC Command Register 1.
 */
#define HW_SSP_CMD1	0x020

#define HW_SSP_CMD1_CMD_ARG	__BITS(31, 0)

/*
 * SD/MMC Compare Reference.
 */
#define HW_SSP_COMPREF	0x030

#define HW_SSP_COMPREF_REFERENCE	__BITS(31, 0)

/*
 * SD/MMC compare mask.
 */
#define HW_SSP_COMPMASK	0x040

#define HW_SSP_COMPMASK_MASK	__BITS(31, 0)

/*
 * SSP Timing Register.
 */
#define HW_SSP_TIMING	0x050

#define HW_SSP_TIMING_TIMEOUT		__BITS(31, 16)
#define HW_SSP_TIMING_CLOCK_DIVIDE	__BITS(15, 8)
#define HW_SSP_TIMING_CLOCK_RATE	__BITS(7, 0)

/*
 * SSP Control Register 1.
 */
#define HW_SSP_CTRL1		0x060
#define HW_SSP_CTRL1_SET	0x064
#define HW_SSP_CTRL1_CLR	0x068
#define HW_SSP_CTRL1_TOG	0x06C

#define HW_SSP_CTRL1_SDIO_IRQ			__BIT(31)
#define HW_SSP_CTRL1_SDIO_IRQ_EN		__BIT(30)
#define HW_SSP_CTRL1_RESP_ERR_IRQ		__BIT(29)
#define HW_SSP_CTRL1_RESP_ERR_IRQ_EN		__BIT(28)
#define HW_SSP_CTRL1_RESP_TIMEOUT_IRQ		__BIT(27)
#define HW_SSP_CTRL1_RESP_TIMEOUT_IRQ_EN	__BIT(26)
#define HW_SSP_CTRL1_DATA_TIMEOUT_IRQ		__BIT(25)
#define HW_SSP_CTRL1_DATA_TIMEOUT_IRQ_EN	__BIT(24)
#define HW_SSP_CTRL1_DATA_CRC_IRQ		__BIT(23)
#define HW_SSP_CTRL1_DATA_CRC_IRQ_EN		__BIT(22)
#define HW_SSP_CTRL1_FIFO_UNDERRUN_IRQ		__BIT(21)
#define HW_SSP_CTRL1_FIFO_UNDERRUN_EN		__BIT(20)
#define HW_SSP_CTRL1_RSVD3			__BIT(19)
#define HW_SSP_CTRL1_RSVD2			__BIT(18)
#define HW_SSP_CTRL1_RECV_TIMEOUT_IRQ		__BIT(17)
#define HW_SSP_CTRL1_RECV_TIMEOUT_IRQ_EN	__BIT(16)
#define HW_SSP_CTRL1_FIFO_OVERRUN_IRQ		__BIT(15)
#define HW_SSP_CTRL1_FIFO_OVERRUN_IRQ_EN	__BIT(14)
#define HW_SSP_CTRL1_DMA_ENABLE			__BIT(13)
#define HW_SSP_CTRL1_RSVD1			__BIT(12)
#define HW_SSP_CTRL1_SLAVE_OUT_DISABLE		__BIT(11)
#define HW_SSP_CTRL1_PHASE			__BIT(10)
#define HW_SSP_CTRL1_POLARITY			__BIT(9)
#define HW_SSP_CTRL1_SLAVE_MODE			__BIT(8)
#define HW_SSP_CTRL1_WORD_LENGTH		__BITS(7, 4)
#define HW_SSP_CTRL1_SSP_MODE			__BITS(3, 0)

/*
 * SSP Data Register.
 */
#define HW_SSP_DATA	0x070

#define HW_SSP_DATA_DATA	__BITS(31, 0)

/*
 * SD/MMC Card Response Register 0.
 */
#define HW_SSP_SDRESP0	0x080

#define HW_SSP_SDRESP0_RESP0	__BITS(31, 0)

/*
 * SD/MMC Card Response Register 1.
 */
#define HW_SSP_SDRESP1	0x090

#define HW_SSP_SDRESP1_RESP1	__BITS(31, 0)

/*
 * SD/MMC Card Response Register 2.
 */
#define HW_SSP_SDRESP2	0x0A0

#define HW_SSP_SDRESP2_RESP2	__BITS(31, 0)

/*
 * SD/MMC Card Response Register 3.
 */
#define HW_SSP_SDRESP3	0x0B0

#define HW_SSP_SDRESP3_RESP3	__BITS(31, 0)

/*
 * SSP Status Register.
 */
#define HW_SSP_STATUS	0x0C0

#define HW_SSP_STATUS_PRESENT		__BIT(31)
#define HW_SSP_STATUS_RSVD5		__BIT(30)
#define HW_SSP_STATUS_SD_PRESENT	__BIT(29)
#define HW_SSP_STATUS_CARD_DETECT	__BIT(28)
#define HW_SSP_STATUS_RSVD4		__BITS(27, 22)
#define HW_SSP_STATUS_DMASENSE		__BIT(21)
#define HW_SSP_STATUS_DMATERM		__BIT(20)
#define HW_SSP_STATUS_DMAREQ		__BIT(19)
#define HW_SSP_STATUS_DMAEND		__BIT(18)
#define HW_SSP_STATUS_SDIO_IRQ		__BIT(17)
#define HW_SSP_STATUS_RESP_CRC_ERR	__BIT(16)
#define HW_SSP_STATUS_RESP_ERR		__BIT(15)
#define HW_SSP_STATUS_RESP_TIMEOUT	__BIT(14)
#define HW_SSP_STATUS_DATA_CRC_ERR	__BIT(13)
#define HW_SSP_STATUS_TIMEOUT		__BIT(12)
#define HW_SSP_STATUS_RECV_TIMEOUT_STAT	__BIT(11)
#define HW_SSP_STATUS_RSVD3		__BIT(10)
#define HW_SSP_STATUS_FIFO_OVRFLW	__BIT(9)
#define HW_SSP_STATUS_FIFO_FULL		__BIT(8)
#define HW_SSP_STATUS_RSVD2		__BIT(7, 6)
#define HW_SSP_STATUS_FIFO_EMPTY	__BIT(5)
#define HW_SSP_STATUS_FIFO_UNDRFLW	__BIT(4)
#define HW_SSP_STATUS_CMD_BUSY		__BIT(3)
#define HW_SSP_STATUS_DATA_BUSY		__BIT(2)
#define HW_SSP_STATUS_RSVD1		__BIT(1)
#define HW_SSP_STATUS_BUSY		__BIT(0)

/*
 * SSP Debug Register.
 */
#define HW_SSP_DEBUG	0x100

#define HW_SSP_DEBUG_DATACRC_ERR	__BITS(31, 28)
#define HW_SSP_DEBUG_DATA_STALL		__BIT(27)
#define HW_SSP_DEBUG_DAT_SM		__BITS(26, 24)
#define HW_SSP_DEBUG_RSVD1		__BITS(23, 20)
#define HW_SSP_DEBUG_CMD_OE		__BIT(19)
#define HW_SSP_DEBUG_DMA_SM		__BITS(18, 16)
#define HW_SSP_DEBUG_MMC_SM		__BITS(15, 12)
#define HW_SSP_DEBUG_CMD_SM		__BITS(11, 10)
#define HW_SSP_DEBUG_SSP_CMD		__BIT(9)
#define HW_SSP_DEBUG_SSP_RESP		__BIT(8)
#define HW_SSP_DEBUG_SSP_RXD		__BITS(7, 0)

/*
 * SSP Version Register.
 */
#define HW_SSP_VERSION	0x110

#define HW_SSP_VERSION_MAJOR	__BITS(31, 24)
#define HW_SSP_VERSION_MINOR	__BITS(23, 16)
#define HW_SSP_VERSION_STEP	__BITS(15, 0)

#endif /* !_ARM_IMX_IMX23_SSPREG_H_ */
