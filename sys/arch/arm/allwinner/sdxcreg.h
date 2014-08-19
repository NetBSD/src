/* $NetBSD: sdxcreg.h,v 1.1.12.2 2014/08/20 00:02:44 tls Exp $ */
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _ARM_ALLWINNER_SDXCREG_H_
#define _ARM_ALLWINNER_SDXCREG_H_

/* SDXC definitions */
#define SDXC_GCTRL_REG			0x0000
#define SDXC_CLKCR_REG			0x0004
#define SDXC_TIMEOUT_REG		0x0008
#define SDXC_WIDTH_REG			0x000c
#define SDXC_BLKSZ_REG			0x0010
#define SDXC_BYTECNT_REG		0x0014
#define SDXC_CMD_REG			0x0018
#define SDXC_ARG_REG			0x001c
#define SDXC_RESP0_REG			0x0020
#define SDXC_RESP1_REG			0x0024
#define SDXC_RESP2_REG			0x0028
#define SDXC_RESP3_REG			0x002c
#define SDXC_IMASK_REG			0x0030
#define SDXC_MINT_REG			0x0034
#define SDXC_RINT_REG			0x0038
#define SDXC_STATUS_REG			0x003c
#define SDXC_FTRGLVL_REG		0x0040
#define SDXC_FUNCSEL_REG		0x0044
#define SDXC_CBCR_REG			0x0048
#define SDXC_BBCR_REG			0x004c
#define SDXC_DMAC_REG			0x0080
#define SDXC_DLBA_REG			0x0084
#define SDXC_IDST_REG			0x0088
#define SDXC_IDIE_REG			0x008c
#define SDXC_CHDA_REG			0x0090
#define SDXC_CBDA_REG			0x0094
#define SDXC_FIFO_REG			0x0100

#define	SDXC_GCTRL_AHB_ACCESS		__BIT(31)
#define	SDXC_GCTRL_DDR_MODE		__BIT(10)
#define	SDXC_GCTRL_POS_EDGE_LATCH_DATA	__BIT(9)
#define	SDXC_GCTRL_DEBOUNCE_ENABLE	__BIT(8)
#define	SDXC_GCTRL_DMA_ENABLE		__BIT(5)
#define	SDXC_GCTRL_INT_ENABLE		__BIT(4)
#define	SDXC_GCTRL_DMA_RESET		__BIT(2)
#define	SDXC_GCTRL_FIFO_RESET		__BIT(1)
#define	SDXC_GCTRL_SOFT_RESET		__BIT(0)

#define	SDXC_CARD_LOW_POWER_ON		__BIT(17)
#define	SDXC_CARD_CLK_ON		__BIT(16)

#define SDXC_BUS_WIDTH_1		(0)
#define SDXC_BUS_WIDTH_4		(1)
#define SDXC_BUS_WIDTH_8		(2)

#define SDXC_CMD_START			__BIT(31)
#define SDXC_CMD_VOL_SWITCH		__BIT(27)
#define SDXC_CMD_DISBALE_BOOT		__BIT(26)
#define SDXC_CMD_BOOT_ACK_EXP		__BIT(25)
#define SDXC_CMD_ALT_BOOT_OP		__BIT(24)
#define SDXC_CMD_END_BOOT		__BIT(24)
#define SDXC_CMD_CCS_EXP		__BIT(23)
#define SDXC_CMD_RD_CE_ATA_DEV		__BIT(22)
#define SDXC_CMD_UP_CLK_ONLY		__BIT(21)
#define SDXC_CMD_SEND_INIT_SEQ		__BIT(15)
#define SDXC_CMD_STOP_ABORT_CMD		__BIT(11)
#define SDXC_CMD_WAIT_PRE_OVER		__BIT(11)
#define SDXC_CMD_SEND_AUTO_STOP		__BIT(11)
#define SDXC_CMD_SEQ_MODE		__BIT(11)
#define SDXC_CMD_WRITE			__BIT(10)
#define SDXC_CMD_DATA_EXP		__BIT(9)
#define SDXC_CMD_CHECK_RESP_CRC		__BIT(8)
#define SDXC_CMD_LONG_RESP		__BIT(7)
#define SDXC_CMD_RESP_EXP		__BIT(6)

#define SDXC_INT_ERR_SUM		( SDXC_INT_END_BIT_ERR \
					| SDXC_INT_START_BIT_ERR \
					| SDXC_INT_HARDWARE_LOCKED \
					| SDXC_INT_FIFO_RUN_ERR \
					| SDXC_INT_DATA_TIMEOUT \
					| SDXC_INT_RESP_TIMEOUT \
					| SDXC_INT_RESP_CRC_ERR \
					| SDXC_INT_DATA_CRC_ERR \
					| SDXC_INT_RESP_ERR )
#define SDXC_INT_CARD_REMOVE		__BIT(31)
#define SDXC_INT_CARD_INSERT		__BIT(30)
#define SDXC_INT_SDIO_INT		__BIT(16)
#define SDXC_INT_END_BIT_ERR		__BIT(15)
#define SDXC_INT_AUTO_CMD_DONE		__BIT(14)
#define SDXC_INT_START_BIT_ERR		__BIT(13)
#define SDXC_INT_HARDWARE_LOCKED	__BIT(12)
#define SDXC_INT_FIFO_RUN_ERR		__BIT(11)
#define SDXC_INT_VOL_CHG_DONE		__BIT(10)
#define SDXC_INT_DATA_STARVE		__BIT(10)
#define SDXC_INT_BOOT_START		__BIT(9)
#define SDXC_INT_DATA_TIMEOUT		__BIT(9)
#define SDXC_INT_ACK_RCV		__BIT(8)
#define SDXC_INT_RESP_TIMEOUT		__BIT(8)
#define SDXC_INT_DATA_CRC_ERR		__BIT(7)
#define SDXC_INT_RESP_CRC_ERR		__BIT(6)
#define SDXC_INT_RX_DATA_REQ		__BIT(5)
#define SDXC_INT_TX_DATA_REQ		__BIT(4)
#define SDXC_INT_DATA_OVER		__BIT(3)
#define SDXC_INT_CMD_DONE		__BIT(2)
#define SDXC_INT_RESP_ERR		__BIT(1)

#endif /* _ARM_ALLWINNER_SDXCREG_H_ */
