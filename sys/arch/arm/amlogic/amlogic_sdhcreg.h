/* $NetBSD: amlogic_sdhcreg.h,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

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

#ifndef _ARM_AMLOGIC_SDHCREG_H
#define _ARM_AMLOGIC_SDHCREG_H

#define SD_ARGU_REG		0x00
#define SD_SEND_REG		0x04
#define SD_CNTL_REG		0x08
#define SD_STAT_REG		0x0c
#define SD_CLKC_REG		0x10
#define SD_ADDR_REG		0x14
#define SD_PDMA_REG		0x18
#define SD_MISC_REG		0x1c
#define SD_DATA_REG		0x20
#define SD_ICTL_REG		0x24
#define SD_ISTA_REG		0x28
#define SD_SRST_REG		0x2c
#define SD_ESTA_REG		0x30
#define SD_ENHC_REG		0x34
#define SD_CLK2_REG		0x38

#define SD_SEND_TOTAL_PACK		__BITS(31,16)
#define SD_SEND_R1B			__BIT(12)
#define SD_SEND_DATA_STOP		__BIT(11)
#define SD_SEND_DATA_DIRECTION		__BIT(10)
#define SD_SEND_RESPONSE_NO_CRC		__BIT(9)
#define SD_SEND_RESPONSE_LENGTH		__BIT(8)
#define SD_SEND_COMMAND_HAS_DATA	__BIT(7)
#define SD_SEND_COMMAND_HAS_RESP	__BIT(6)
#define SD_SEND_COMMAND_INDEX		__BITS(5,0)

#define SD_CNTL_TX_ENDIAN_CTRL		__BITS(31,29)
#define SD_CNTL_DAT0_INT_SEL		__BIT(28)
#define SD_CNTL_SDIO_IRQ_MODE		__BIT(27)
#define SD_CNTL_RX_ENDIAN_CTRL		__BITS(26,24)
#define SD_CNTL_RX_PERIOD		__BITS(23,20)
#define SD_CNTL_RX_TIMEOUT		__BITS(19,13)
#define SD_CNTL_PACK_LEN		__BITS(12,4)
#define SD_CNTL_TX_CRC_CHECK		__BIT(3)
#define SD_CNTL_DDR_MODE		__BIT(2)
#define SD_CNTL_DAT_TYPE		__BITS(1,0)

#define SD_STAT_DAT_HI			__BITS(23,20)
#define SD_STAT_TXFIFO_COUNT		__BITS(19,13)
#define SD_STAT_RXFIFO_COUNT		__BITS(12,6)
#define SD_STAT_CMD			__BIT(5)
#define SD_STAT_DAT_LO			__BITS(4,1)
#define SD_STAT_BUSY			__BIT(0)

#define SD_CLKC_MEM_PWR			__BITS(26,25)
#define SD_CLKC_MEM_PWR_ON		0
#define SD_CLKC_MEM_PWR_OFF		3
#define SD_CLKC_CLK_JIC			__BIT(24)
#define SD_CLKC_CLK_IN_SEL		__BITS(17,16)
#define SD_CLKC_CLK_IN_SEL_OSC		0
#define SD_CLKC_CLK_IN_SEL_FCLK_DIV4	1
#define SD_CLKC_CLK_IN_SEL_FCLK_DIV3	2
#define SD_CLKC_CLK_IN_SEL_FCLK_DIV5	3
#define SD_CLKC_MOD_CLK_ENABLE		__BIT(15)
#define SD_CLKC_SD_CLK_ENABLE		__BIT(14)
#define SD_CLKC_RX_CLK_ENABLE		__BIT(13)
#define SD_CLKC_TX_CLK_ENABLE		__BIT(12)
#define SD_CLKC_CLK_DIV			__BITS(11,0)

#define SD_PDMA_TXFIFO_FILL		__BIT(31)
#define SD_PDMA_RXFIFO_MANUAL_FLUSH	__BITS(30,29)
#define SD_PDMA_TXFIFO_THRESHOLD	__BITS(28,22)
#define SD_PDMA_RXFIFO_THRESHOLD	__BITS(21,15)
#define SD_PDMA_RX_BURST_LEN		__BITS(14,10)
#define SD_PDMA_TX_BURST_LEN		__BITS(9,5)
#define SD_PDMA_DMA_URGENT		__BIT(4)
#define SD_PDMA_PIO_RDRESP		__BITS(3,1)
#define SD_PDMA_DMA_MODE		__BIT(0)

#define SD_MISC_TXSTART_THRESHOLD	__BITS(31,29)
#define SD_MISC_STOP_MODE		__BIT(28)
#define SD_MISC_THREAD_ID		__BITS(27,22)
#define SD_MISC_BURST_NUMBER		__BITS(21,16)
#define SD_MISC_WCRC_OK_PATTERN		__BITS(9,7)
#define SD_MISC_WCRC_ERR_PATTERN	__BITS(6,4)

#define SD_ICTL_SDIO_DAT1		__BITS(17,16)

#define SD_INT_ADDL_SDIO_DAT1		__BIT(14)
#define SD_INT_TXFIFO			__BIT(13)
#define SD_INT_RXFIFO			__BIT(12)
#define SD_INT_DMA_DONE			__BIT(11)
#define SD_INT_SDIO_DAT1		__BIT(10)
#define SD_INT_TXFIFO_THRES		__BIT(9)
#define SD_INT_RXFIFO_THRES		__BIT(8)
#define SD_INT_DATA_COMPLETE		__BIT(7)
#define SD_INT_PACK_CRC_ERROR		__BIT(6)
#define SD_INT_PACK_TIMEOUT		__BIT(5)
#define SD_INT_PACK_COMPLETE		__BIT(4)
#define SD_INT_DATA_BIT0_CHG		__BIT(3)
#define SD_INT_RESP_CRC_ERROR		__BIT(2)
#define SD_INT_RESP_TIMEOUT		__BIT(1)
#define SD_INT_RESP_COMPLETE		__BIT(0)

#define SD_INT_TIMEOUT		(SD_INT_PACK_TIMEOUT | SD_INT_RESP_TIMEOUT)
#define SD_INT_CRC_ERROR	(SD_INT_PACK_CRC_ERROR | SD_INT_RESP_CRC_ERROR)
#define SD_INT_ERROR		(SD_INT_TIMEOUT | SD_INT_CRC_ERROR)
#define SD_INT_CLEAR		0x7fff

#define SD_SRST_DMA_IF			__BIT(5)
#define SD_SRST_DPHY_TX			__BIT(4)
#define SD_SRST_DPHY_RX			__BIT(3)
#define SD_SRST_TX_FIFO			__BIT(2)
#define SD_SRST_RX_FIFO			__BIT(1)
#define SD_SRST_MAIN_CTRL		__BIT(0)

#define SD_ENHC_RX_TIMEOUT		__BITS(7,0)
#define SD_ENHC_SDIO_IRQ_PERIOD		__BITS(15,8)
#define SD_ENHC_DMA_RX_RESP		__BIT(16)
#define SD_ENHC_DMA_TX_RESP		__BIT(17)
#define SD_ENHC_RXFIFO_THRESHOLD	__BITS(24,18)
#define SD_ENHC_TXFIFO_THRESHOLD	__BITS(31,25)

#define SD_ESTA_BUSY			__BITS(13,11)	/* XXX ??? */

#define SD_CLK2_RX_CLK_PHASE		__BITS(11,0)
#define SD_CLK2_SD_CLK_PHASE		__BITS(23,12)

#endif /* _ARM_AMLOGIC_SDHCREG_H */
