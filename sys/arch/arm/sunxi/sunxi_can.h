/*	$NetBSD: sunxi_can.h,v 1.1 2018/03/07 20:55:31 bouyer Exp $	*/

/*-
 * Copyright (c) 2017,2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

/* CAN mode select register */
#define SUNXI_CAN_MODSEL_REG		0x00
#define SUNXI_CAN_MODSEL_SLEEP		__BIT(4)
#define SUNXI_CAN_MODSEL_ACP_FLT_MOD	__BIT(3)
#define SUNXI_CAN_MODSEL_LB_MOD		__BIT(2)
#define SUNXI_CAN_MODSEL_LST_ONLY	__BIT(1)
#define SUNXI_CAN_MODSEL_RST		__BIT(0)

/* CAN command register */
#define SUNXI_CAN_CMD_REG	0x04
#define SUNXI_CAN_CMD_BUS_OFF		__BIT(5)
#define SUNXI_CAN_CMD_SELF_REQ		__BIT(4)
#define SUNXI_CAN_CMD_CLR_OR		__BIT(3)
#define SUNXI_CAN_CMD_REL_RX_BUF		__BIT(2)
#define SUNXI_CAN_CMD_ABT_REQ		__BIT(1)
#define SUNXI_CAN_CMD_TANS_REQ		__BIT(0)

/* CAN status register */
#define SUNXI_CAN_STA_REG	0x08
#define SUNXI_CAN_STA_ERR_CODE		__BITS(23,22)
#define SUNXI_CAN_STA_ERR_CODE_BIT		0
#define SUNXI_CAN_STA_ERR_CODE_FORM		1
#define SUNXI_CAN_STA_ERR_CODE_STUFF		2
#define SUNXI_CAN_STA_ERR_CODE_OTHER		3
#define SUNXI_CAN_STA_ERR_DIR		_BIT(21)
#define SUNXI_CAN_STA_ERR_SEG_CODE	__BITS(20,16)
#define SUNXI_CAN_STA_ARB_LOST		__BITS(12,8)
#define SUNXI_CAN_STA_BUS		__BIT(7)
#define SUNXI_CAN_STA_ERR		__BIT(6)
#define SUNXI_CAN_STA_TX 		__BIT(5)
#define SUNXI_CAN_STA_RX 		__BIT(4)
#define SUNXI_CAN_STA_TX_OVER		__BIT(3)
#define SUNXI_CAN_STA_TX_RDY		__BIT(2)
#define SUNXI_CAN_STA_DATA_OR		__BIT(1)
#define SUNXI_CAN_STA_RX_RDY		__BIT(0)

/* CAN interrupt register */
#define SUNXI_CAN_INT_REG	0x0c
#define SUNXI_CAN_INT_BERR		__BIT(7)
#define SUNXI_CAN_INT_ARB_LOST		__BIT(6)
#define SUNXI_CAN_INT_ERR_PASSIVE	__BIT(5)
#define SUNXI_CAN_INT_WAKEUP		__BIT(4)
#define SUNXI_CAN_INT_DATA_OR		__BIT(3)
#define SUNXI_CAN_INT_ERR		__BIT(2)
#define SUNXI_CAN_INT_TX_FLAG		__BIT(1)
#define SUNXI_CAN_INT_RX_FLAG		__BIT(0)

/* CAN interrupt enable register */
#define SUNXI_CAN_INTE_REG	0x10

/* CAN bus timing register */
#define SUNXI_CAN_BUS_TIME_REG	0x14
#define SUNXI_CAN_BUS_TIME_SAM		__BIT(23)
#define SUNXI_CAN_BUS_TIME_PHSEG2	__BITS(22,20)
#define SUNXI_CAN_BUS_TIME_PHSEG1	__BITS(19,16)
#define SUNXI_CAN_BUS_TIME_SJW		__BITS(15,14)
#define SUNXI_CAN_BUS_TIME_TQ_BRP	__BITS(9,0)

/* CAN tx error warning limit register */
#define SUNXI_CAN_EWL_REG	0x18
#define SUNXI_CAN_EWL_ERR_WRN_LMT	__BITS(7,0)

/* CAN error counter register */
#define SUNXI_CAN_REC_REG	0x1c
#define SUNXI_CAN_REC_RX_ERR_CNT		__BITS(23,16)
#define SUNXI_CAN_REC_TX_ERR_CNT		__BITS(7,0)

/* CAN receive message register */
#define SUNXI_CAN_RMSGC_REG	0x20
#define SUNXI_CAN_RMSGC_RX_MSG_CNT	__BITS(7,0)

/* CAN receive buffer start address register */
#define SUNXI_CAN_RSADDR_REG	0x24
#define SUNXI_CAN_RSADDR_RX_BUF_SADDR	__BITS(5,0)

/* CAN rx/tx message buffer 0 register */
#define SUNXI_CAN_TXBUF0_REG	0x40
#define SUNXI_CAN_TXBUF0_EFF		__BIT(7)
#define SUNXI_CAN_TXBUF0_RTR		__BIT(6)
#define SUNXI_CAN_TXBUF0_DL		__BITS(3,0)

/* CAN rx/tx message buffer registers */
#define SUNXI_CAN_TXBUF1_REG	0x44
#define SUNXI_CAN_TXBUF2_REG	0x48
#define SUNXI_CAN_TXBUF3_REG	0x4c
#define SUNXI_CAN_TXBUF4_REG	0x50
#define SUNXI_CAN_TXBUF5_REG	0x54
#define SUNXI_CAN_TXBUF6_REG	0x58
#define SUNXI_CAN_TXBUF7_REG	0x5c
#define SUNXI_CAN_TXBUF8_REG	0x60
#define SUNXI_CAN_TXBUF9_REG	0x64
#define SUNXI_CAN_TXBUF10_REG	0x68
#define SUNXI_CAN_TXBUF11_REG	0x6c
#define SUNXI_CAN_TXBUF12_REG	0x70

/* CAN acceptance code 0 register */
#define SUNXI_CAN_ACPC		0x40

/* CAN acceptance mask 0 register */
#define SUNXI_CAN_ACPM		0x44

/* CAN transmit buffer for read back registers */
#define SUNXI_CAN_RBUF_RBACK0	0x180
#define SUNXI_CAN_RBUF_RBACK1	0x184
#define SUNXI_CAN_RBUF_RBACK2	0x188
#define SUNXI_CAN_RBUF_RBACK3	0x18c
#define SUNXI_CAN_RBUF_RBACK4	0x190
#define SUNXI_CAN_RBUF_RBACK5	0x194
#define SUNXI_CAN_RBUF_RBACK6	0x198
#define SUNXI_CAN_RBUF_RBACK7	0x19c
#define SUNXI_CAN_RBUF_RBACK8	0x1a0
#define SUNXI_CAN_RBUF_RBACK9	0x1a4
#define SUNXI_CAN_RBUF_RBACK10	0x1a8
#define SUNXI_CAN_RBUF_RBACK11	0x1ac
#define SUNXI_CAN_RBUF_RBACK12	0x1b0
