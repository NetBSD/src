/* $NetBSD: sunxi_rsb.h,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_SUNXI_RSB_H
#define _ARM_SUNXI_RSB_H

#define RSB_CTRL_REG			0x0000
#define  RSB_CTRL_START_TRANS		__BIT(7)
#define  RSB_CTRL_ABORT_TRANS		__BIT(6)
#define  RSB_CTRL_GLOBAL_INT_ENB	__BIT(1)
#define  RSB_CTRL_SOFT_RESET		__BIT(0)
#define RSB_CCR_REG			0x0004
#define  RSB_CCR_SDA_ODLY		__BITS(10,8)
#define  RSB_CCR_CLK_DIV		__BITS(7,0)
#define RSB_INTE_REG			0x0008
#define  RSB_INTE_LOAD_BSY_ENB		__BIT(2)
#define  RSB_INTE_TRANS_ERR_ENB		__BIT(1)
#define  RSB_INTE_TRANS_OVER_ENB	__BIT(0)
#define RSB_STAT_REG			0x000c
#define  RSB_STAT_TRANS_ERR_ID		__BITS(15,8)
#define  RSB_STAT_LOAD_BSY		__BIT(2)
#define  RSB_STAT_TRANS_ERR		__BIT(1)
#define  RSB_STAT_TRANS_OVER		__BIT(0)
#define  RSB_STAT_MASK		\
	(RSB_STAT_LOAD_BSY |	\
	 RSB_STAT_TRANS_ERR |	\
	 RSB_STAT_TRANS_OVER)
#define RSB_DADDR0_REG			0x0010
#define RSB_DADDR1_REG			0x0014
#define RSB_DLEN_REG			0x0018
#define  RSB_DLEN_READ_WRITE_FLAG	__BIT(4)
#define  RSB_DLEN_ACCESS_LENGTH		__BITS(2,0)
#define RSB_DATA0_REG			0x001c
#define RSB_DATA1_REG			0x0020
#define RSB_LCR_REG			0x0024
#define  RSB_LCR_SCL_STATE		__BIT(5)
#define  RSB_LCR_SDA_STATE		__BIT(4)
#define  RSB_LCR_SCL_CTL		__BIT(3)
#define  RSB_LCR_SCL_CTL_EN		__BIT(2)
#define  RSB_LCR_SDA_CTL		__BIT(1)
#define  RSB_LCR_SDA_CTL_EN		__BIT(0)
#define RSB_PMCR_REG			0x0028
#define  RSB_PMCR_PMU_INIT_SEND		__BIT(31)
#define  RSB_PMCR_PMU_INIT_DATA		__BITS(23,16)
#define  RSB_PMCR_PMU_MODE_CTRL_REG_ADDR __BITS(15,8)
#define  RSB_PMCR_PMU_DEVICE_ADDR	__BITS(7,0)
#define RSB_CMD_REG			0x002c
#define  RSB_CMD_IDX			__BITS(7,0)
#define   RSB_CMD_IDX_SRTA		0xe8
#define   RSB_CMD_IDX_RD8		0x8b
#define   RSB_CMD_IDX_RD16		0x9c
#define   RSB_CMD_IDX_RD32		0xa6
#define   RSB_CMD_IDX_WR8		0x4e
#define   RSB_CMD_IDX_WR16		0x59
#define   RSB_CMD_IDX_WR32		0x63
#define RSB_DAR_REG			0x0030
#define  RSB_DAR_RTA			__BITS(23,16)
#define  RSB_DAR_DA			__BITS(15,0)

#endif /* _ARM_SUNXI_RSB_H */
