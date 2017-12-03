/*	$NetBSD: octeon_gmxreg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * GMX Registers
 */

#ifndef _OCTEON_GMXREG_H_
#define _OCTEON_GMXREG_H_

#define	GMX0_RX0_INT_REG			0x000
#define	GMX0_RX0_INT_EN				0x008
#define	GMX0_PRT0_CFG				0x010
#define	GMX0_RX0_FRM_CTL			0x018
#define	GMX0_RX0_FRM_CHK			0x020
#define	GMX0_RX0_FRM_MIN			0x028
#define	GMX0_RX0_FRM_MAX			0x030
#define	GMX0_RX0_JABBER				0x038
#define	GMX0_RX0_DECISION			0x040
#define	GMX0_RX0_UDD_SKP			0x048
#define	GMX0_RX0_STATS_CTL			0x050
#define	GMX0_RX0_IFG				0x058
#define	GMX0_RX0_RX_INBND			0x060
#define	GMX0_RX0_STATS_PKTS			0x080
#define	GMX0_RX0_STATS_OCTS			0x088
#define	GMX0_RX0_STATS_PKTS_CTL			0x090
#define	GMX0_RX0_STATS_OCTS_CTL			0x098
#define	GMX0_RX0_STATS_PKTS_DMAC		0x0a0
#define	GMX0_RX0_STATS_OCTS_DMAC		0x0a8
#define	GMX0_RX0_STATS_PKTS_DRP			0x0b0
#define	GMX0_RX0_STATS_OCTS_DRP			0x0b8
#define	GMX0_RX0_STATS_PKTS_BAD			0x0c0
#define	GMX0_RX0_ADR_CTL			0x100
#define	GMX0_RX0_ADR_CAM_EN			0x108
#define	GMX0_RX0_ADR_CAM0			0x180
#define	GMX0_RX0_ADR_CAM1			0x188
#define	GMX0_RX0_ADR_CAM2			0x190
#define	GMX0_RX0_ADR_CAM3			0x198
#define	GMX0_RX0_ADR_CAM4			0x1a0
#define	GMX0_RX0_ADR_CAM5			0x1a8
#define	GMX0_TX0_CLK				0x208
#define	GMX0_TX0_THRESH				0x210
#define	GMX0_TX0_APPEND				0x218
#define	GMX0_TX0_SLOT				0x220
#define	GMX0_TX0_BURST				0x228
#define	GMX0_SMAC0				0x230
#define	GMX0_TX0_PAUSE_PKT_TIME			0x238
#define	GMX0_TX0_MIN_PKT			0x240
#define	GMX0_TX0_PAUSE_PKT_INTERVAL		0x248
#define	GMX0_TX0_SOFT_PAUSE			0x250
#define	GMX0_TX0_PAUSE_TOGO			0x258
#define	GMX0_TX0_PAUSE_ZERO			0x260
#define	GMX0_TX0_STATS_CTL			0x268
#define	GMX0_TX0_CTL				0x270
#define	GMX0_TX0_STAT0				0x280
#define	GMX0_TX0_STAT1				0x288
#define	GMX0_TX0_STAT2				0x290
#define	GMX0_TX0_STAT3				0x298
#define	GMX0_TX0_STAT4				0x2a0
#define	GMX0_TX0_STAT5				0x2a8
#define	GMX0_TX0_STAT6				0x2b0
#define	GMX0_TX0_STAT7				0x2b8
#define	GMX0_TX0_STAT8				0x2c0
#define	GMX0_TX0_STAT9				0x2c8
#define	GMX0_BIST0				0x400
#define	GMX0_RX_PRTS				0x410
#define	GMX0_RX_BP_DROP0			0x420
#define	GMX0_RX_BP_DROP1			0x428
#define	GMX0_RX_BP_DROP2			0x430
#define	GMX0_RX_BP_ON0				0x440
#define	GMX0_RX_BP_ON1				0x448
#define	GMX0_RX_BP_ON2				0x450
#define	GMX0_RX_BP_OFF0				0x460
#define	GMX0_RX_BP_OFF1				0x468
#define	GMX0_RX_BP_OFF2				0x470
#define	GMX0_TX_PRTS				0x480
#define	GMX0_TX_IFG				0x488
#define	GMX0_TX_JAM				0x490
#define	GMX0_TX_COL_ATTEMPT			0x498
#define	GMX0_TX_PAUSE_PKT_DMAC			0x4a0
#define	GMX0_TX_PAUSE_PKT_TYPE			0x4a8
#define	GMX0_TX_OVR_BP				0x4c8
#define	GMX0_TX_BP				0x4d0
#define	GMX0_TX_CORRUPT				0x4d8
#define	GMX0_RX_PRT_INFO			0x4e8
#define	GMX0_TX_LFSR				0x4f8
#define	GMX0_TX_INT_REG				0x500
#define	GMX0_TX_INT_EN				0x508
#define	GMX0_NXA_ADR				0x510
#define	GMX0_BAD_REG				0x518
#define	GMX0_STAT_BP				0x520
#define	GMX0_TX_CLK_MSK0			0x780
#define	GMX0_TX_CLK_MSK1			0x788
#define	GMX0_RX_TX_STATUS			0x7e8
#define	GMX0_INF_MODE				0x7f8

/* -------------------------------------------------------------------------- */

/* GMX Interrupt Registers */

#define	RXN_INT_REG_XXX_63_19			UINT64_C(0xfffffffffff80000)
#define	RXN_INT_REG_PHY_DUPX			UINT64_C(0x0000000000040000)
#define	RXN_INT_REG_PHY_SPD			UINT64_C(0x0000000000020000)
#define	RXN_INT_REG_PHY_LINK			UINT64_C(0x0000000000010000)
#define	RXN_INT_REG_IFGERR			UINT64_C(0x0000000000008000)
#define	RXN_INT_REG_COLDET			UINT64_C(0x0000000000004000)
#define	RXN_INT_REG_FALERR			UINT64_C(0x0000000000002000)
#define	RXN_INT_REG_RSVERR			UINT64_C(0x0000000000001000)
#define	RXN_INT_REG_PCTERR			UINT64_C(0x0000000000000800)
#define	RXN_INT_REG_OVRERR			UINT64_C(0x0000000000000400)
#define	RXN_INT_REG_NIBERR			UINT64_C(0x0000000000000200)
#define	RXN_INT_REG_SKPERR			UINT64_C(0x0000000000000100)
#define	RXN_INT_REG_RCVERR			UINT64_C(0x0000000000000080)
#define	RXN_INT_REG_LENERR			UINT64_C(0x0000000000000040)
#define	RXN_INT_REG_ALNERR			UINT64_C(0x0000000000000020)
#define	RXN_INT_REG_FCSERR			UINT64_C(0x0000000000000010)
#define	RXN_INT_REG_JABBER			UINT64_C(0x0000000000000008)
#define	RXN_INT_REG_MAXERR			UINT64_C(0x0000000000000004)
#define	RXN_INT_REG_CAREXT			UINT64_C(0x0000000000000002)
#define	RXN_INT_REG_MINERR			UINT64_C(0x0000000000000001)

/* GMX Interrupt-Enable Registers */

#define	RXN_INT_EN_XXX_63_19			UINT64_C(0xfffffffffff80000)
#define	RXN_INT_EN_PHY_DUPX			UINT64_C(0x0000000000040000)
#define	RXN_INT_EN_PHY_SPD			UINT64_C(0x0000000000020000)
#define	RXN_INT_EN_PHY_LINK			UINT64_C(0x0000000000010000)
#define	RXN_INT_EN_IFGERR			UINT64_C(0x0000000000008000)
#define	RXN_INT_EN_COLDET			UINT64_C(0x0000000000004000)
#define	RXN_INT_EN_FALERR			UINT64_C(0x0000000000002000)
#define	RXN_INT_EN_RSVERR			UINT64_C(0x0000000000001000)
#define	RXN_INT_EN_PCTERR			UINT64_C(0x0000000000000800)
#define	RXN_INT_EN_OVRERR			UINT64_C(0x0000000000000400)
#define	RXN_INT_EN_NIBERR			UINT64_C(0x0000000000000200)
#define	RXN_INT_EN_SKPERR			UINT64_C(0x0000000000000100)
#define	RXN_INT_EN_RCVERR			UINT64_C(0x0000000000000080)
#define	RXN_INT_EN_LENERR			UINT64_C(0x0000000000000040)
#define	RXN_INT_EN_ALNERR			UINT64_C(0x0000000000000020)
#define	RXN_INT_EN_FCSERR			UINT64_C(0x0000000000000010)
#define	RXN_INT_EN_JABBER			UINT64_C(0x0000000000000008)
#define	RXN_INT_EN_MAXERR			UINT64_C(0x0000000000000004)
#define	RXN_INT_EN_CAREXT			UINT64_C(0x0000000000000002)
#define	RXN_INT_EN_MINERR			UINT64_C(0x0000000000000001)

/* GMX Port Configuration Registers */

#define	PRTN_CFG_XXX_63_4			UINT64_C(0xfffffffffffffff0)
#define	PRTN_CFG_SLOTTIME			UINT64_C(0x0000000000000008)
#define	PRTN_CFG_DUPLEX				UINT64_C(0x0000000000000004)
#define	PRTN_CFG_SPEED				UINT64_C(0x0000000000000002)
#define	PRTN_CFG_EN				UINT64_C(0x0000000000000001)

/* Frame Control Registers */

#define	RXN_FRM_CTL_XXX_63_11			UINT64_C(0xfffffffffffff800)
#define	RXN_FRM_CTL_NULL_DIS			UINT64_C(0x0000000000000400)
#define	RXN_FRM_CTL_PRE_ALIGN			UINT64_C(0x0000000000000200)
#define	RXN_FRM_CTL_PAD_LEN			UINT64_C(0x0000000000000100)
#define	RXN_FRM_CTL_VLAN_LEN			UINT64_C(0x0000000000000080)
#define	RXN_FRM_CTL_PRE_FREE			UINT64_C(0x0000000000000040)
#define	RXN_FRM_CTL_CTL_SMAC			UINT64_C(0x0000000000000020)
#define	RXN_FRM_CTL_CTL_MCST			UINT64_C(0x0000000000000010)
#define	RXN_FRM_CTL_CTL_BCK			UINT64_C(0x0000000000000008)
#define	RXN_FRM_CTL_CTL_DRP			UINT64_C(0x0000000000000004)
#define	RXN_FRM_CTL_PRE_STRP			UINT64_C(0x0000000000000002)
#define	RXN_FRM_CTL_PRE_CHK			UINT64_C(0x0000000000000001)

/* Frame Check Registers */

#define RXN_FRM_CKK_XXX_63_10			UINT64_C(0xfffffffffffffc00)
#define	RXN_FRM_CHK_NIBERR			UINT64_C(0x0000000000000200)
#define	RXN_FRM_CHK_SKPERR			UINT64_C(0x0000000000000100)
#define	RXN_FRM_CHK_RCVERR			UINT64_C(0x0000000000000080)
#define	RXN_FRM_CHK_LENERR			UINT64_C(0x0000000000000040)
#define	RXN_FRM_CHK_ALNERR			UINT64_C(0x0000000000000020)
#define	RXN_FRM_CHK_FCSERR			UINT64_C(0x0000000000000010)
#define	RXN_FRM_CHK_JABBER			UINT64_C(0x0000000000000008)
#define	RXN_FRM_CHK_MAXERR			UINT64_C(0x0000000000000004)
#define	RXN_FRM_CHK_CAREXT			UINT64_C(0x0000000000000002)
#define	RXN_FRM_CHK_MINERR			UINT64_C(0x0000000000000001)

/* Frame Minimum-Length Registers */

#define	RXN_RRM_MIN_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	RXN_RRM_MIN_LEN				UINT64_C(0x000000000000ffff)

/* Frame Maximun-Length Registers */

#define	RXN_RRM_MAX_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	RXN_RRM_MAX_LEN				UINT64_C(0x000000000000ffff)

/* GMX Maximun Packet-Size Registers */

#define	RXN_JABBER_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	RXN_JABBER_CNT				UINT64_C(0x000000000000ffff)

/* GMX Packet Decision Registers */

#define	RXN_DECISION_XXX_63_5			UINT64_C(0xffffffffffffffe0)
#define	RXN_DECISION_CNT			UINT64_C(0x000000000000001f)

/* GMX User-Defined Data Skip Registers */

#define	RXN_UDD_SKP_XXX_63_9			UINT64_C(0xfffffffffffffe00)
#define	RXN_UDD_SKP_FCSSEL			UINT64_C(0x0000000000000100)
#define	RXN_UDD_SKP_XXX_7			UINT64_C(0x0000000000000080)
#define	RXN_UDD_SKP_LEN				UINT64_C(0x000000000000007f)

/* GMX RX Statistics Control Registers */

#define	RXN_STATS_CTL_XXX_63_1			UINT64_C(0xfffffffffffffffe)
#define	RXN_STATS_CTL_RD_CLR			UINT64_C(0x0000000000000001)

/* GMX Minimun Interface-Gap Cycles Registers */

#define	RXN_IFG_XXX_63_4			UINT64_C(0xfffffffffffffff0)
#define	RXN_IFG_IFG				UINT64_C(0x000000000000000f)

/* InBand Link Status Registers */

#define	RXN_RX_INBND_XXX_63_4			UINT64_C(0xfffffffffffffff0)
#define	RXN_RX_INBND_DUPLEX			UINT64_C(0x0000000000000008)
#define	 RXN_RX_INBND_DUPLEX_SHIFT		3
#define	  RXN_RX_INBND_DUPLEX_HALF		(0ULL << RXN_RX_INBND_DUPLEX_SHIFT)
#define	  RXN_RX_INBND_DUPLEX_FULL		(1ULL << RXN_RX_INBND_DUPLEX_SHIFT)
#define	RXN_RX_INBND_SPEED			UINT64_C(0x0000000000000006)
#define	 RXN_RX_INBND_SPEED_SHIFT		1
#define	  RXN_RX_INBND_SPEED_2_5		(0ULL << RXN_RX_INBND_SPEED_SHIFT)
#define	  RXN_RX_INBND_SPEED_25			(1ULL << RXN_RX_INBND_SPEED_SHIFT)
#define	  RXN_RX_INBND_SPEED_125		(2ULL << RXN_RX_INBND_SPEED_SHIFT)
#define	  RXN_RX_INBND_SPEED_XXX_3		(3ULL << RXN_RX_INBND_SPEED_SHIFT)
#define	RXN_RX_INBND_STATUS			UINT64_C(0x0000000000000001)

/* GMX RX Good Packets Registers */

#define	RXN_STATS_PKTS_XXX_63_32		UINT64_C(0xffffffff00000000)
#define	RXN_STATS_PKTS_CNT			UINT64_C(0x00000000ffffffff)

/* GMX RX Good Packets Octet Registers */

#define	RXN_STATS_OCTS_XXX_63_48		UINT64_C(0xffff000000000000)
#define	RXN_STATS_OCTS_CNT			UINT64_C(0x0000ffffffffffff)

/* GMX RX Pause Packets Registers */

#define	RXN_STATS_PKTS_CTL_XXX_63_32		UINT64_C(0xffffffff00000000)
#define	RXN_STATS_PKTS_CTL_CNT			UINT64_C(0x00000000ffffffff)

/* GMX RX Pause Packets Octet Registers */

#define	RXN_STATS_OCTS_CTL_XXX_63_48		UINT64_C(0xffff000000000000)
#define	RXN_STATS_OCTS_CTL_CNT			UINT64_C(0x0000ffffffffffff)

/* GMX RX DMAC Packets Registers */

#define	RXN_STATS_PKTS_DMAC_XXX_63_32		UINT64_C(0xffffffff00000000)
#define	RXN_STATS_PKTS_DMAC_CNT			UINT64_C(0x00000000ffffffff)

/* GMX RX DMAC Packets Octet Registers */

#define	RXN_STATS_OCTS_DMAC_XXX_63_48		UINT64_C(0xffff000000000000)
#define	RXN_STATS_OCTS_DMAC_CNT			UINT64_C(0x0000ffffffffffff)

/* GMX RX Overflow Packets Registers */

#define	RXN_STATS_PKTS_DRP_XXX_63_48		UINT64_C(0xffffffff00000000)
#define	RXN_STATS_PKTS_DRP_CNT			UINT64_C(0x00000000ffffffff)

/* GMX RX Overflow Packets Octet Registers */

#define	RXN_STATS_OCTS_DRP_XXX_63_48		UINT64_C(0xffff000000000000)
#define	RXN_STATS_OCTS_DRP_CNT			UINT64_C(0x0000ffffffffffff)

/* GMX RX Bad Packets Registers */

#define	RXN_STATS_PKTS_BAD_XXX_63_48		UINT64_C(0xffffffff00000000)
#define	RXN_STATS_PKTS_BAD_CNT			UINT64_C(0x00000000ffffffff)

/* Address-Filtering Control Registers */

#define	RXN_ADR_CTL_XXX_63_4			UINT64_C(0xfffffffffffffff0)
#define	RXN_ADR_CTL_CAM_MODE			UINT64_C(0x0000000000000008)
#define	 RXN_ADR_CTL_CAM_MODE_SHIFT		3
#define	  RXN_ADR_CTL_CAM_MODE_REJECT		(0ULL << RXN_ADR_CTL_CAM_MODE_SHIFT)
#define	  RXN_ADR_CTL_CAM_MODE_ACCEPT		(1ULL << RXN_ADR_CTL_CAM_MODE_SHIFT)
#define	RXN_ADR_CTL_MCST			UINT64_C(0x0000000000000006)
#define	 RXN_ADR_CTL_MCST_SHIFT			1
#define	  RXN_ADR_CTL_MCST_AFCAM		(0ULL << RXN_ADR_CTL_MCST_SHIFT)
#define	  RXN_ADR_CTL_MCST_REJECT		(1ULL << RXN_ADR_CTL_MCST_SHIFT)
#define	  RXN_ADR_CTL_MCST_ACCEPT		(2ULL << RXN_ADR_CTL_MCST_SHIFT)
#define	  RXN_ADR_CTL_MCST_XXX_3		(3ULL << RXN_ADR_CTL_MCST_SHIFT)
#define	RXN_ADR_CTL_BCST			UINT64_C(0x0000000000000001)

/* Address-Filtering Control Enable Registers */

#define	RXN_ADR_CAM_EN_XXX_63_8			UINT64_C(0xffffffffffffff00)
#define	RXN_ADR_CAM_EN_EN			UINT64_C(0x00000000000000ff)

/* Address-Filtering CAM Control Registers */
#define	RXN_ADR_CAMN_ADR			UINT64_C(0xffffffffffffffff)

/* GMX TX Clock Generation Registers */

#define	TXN_CLK_XXX_63_6			UINT64_C(0xffffffffffffffc0)
#define	TXN_CLK_CLK_CNT				UINT64_C(0x000000000000003f)

/* TX Threshold Registers */

#define	TXN_THRESH_XXX_63_6			UINT64_C(0xffffffffffffffc0)
#define	TXN_THRESH_CNT				UINT64_C(0x000000000000003f)

/* TX Append Control Registers */

#define	TXN_APPEND_XXX_63_4			UINT64_C(0xfffffffffffffff0)
#define	TXN_APPEND_FORCE_FCS			UINT64_C(0x0000000000000008)
#define	TXN_APPEND_FCS				UINT64_C(0x0000000000000004)
#define	TXN_APPEND_PAD				UINT64_C(0x0000000000000002)
#define	TXN_APPEND_PREAMBLE			UINT64_C(0x0000000000000001)

/* TX Slottime Counter Registers */

#define	TXN_SLOT_XXX_63_10			UINT64_C(0xfffffffffffffc00)
#define	TXN_SLOT_SLOT				UINT64_C(0x00000000000003ff)

/* TX Burst-Counter Registers */

#define	TXN_BURST_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	TXN_BURST_BURST				UINT64_C(0x000000000000ffff)

/* RGMII SMAC Registers */

#define	SMACN_XXX_63_48				UINT64_C(0xffff000000000000)
#define	SMACN_SMAC				UINT64_C(0x0000ffffffffffff)

/* TX Pause Packet Pause-Time Registers */

#define	TXN_PAUSE_PKT_TIME_XXX_63_16		UINT64_C(0xffffffffffff0000)
#define	TXN_PAUSE_PKT_TIME_TIME			UINT64_C(0x000000000000ffff)

/* RGMII TX Minimum-Size-Packet Registers */

#define	TXN_MIN_PKT_XXX_63_8			UINT64_C(0xffffffffffffff00)
#define	TXN_MIN_PKT_MIN_SIZE			UINT64_C(0x00000000000000ff)

/* TX Pause-Packet Transmission-Interval Registers */

#define	TXN_PAUSE_PKT_INTERVAL_XXX_63_16	UINT64_C(0xffffffffffff0000)
#define	TXN_PAUSE_PKT_INTERVAL_INTERVAL		UINT64_C(0x000000000000ffff)

/* TX Software-Pause Registers */

#define	TXN_SOFT_PAUSE_XXX_63_16		UINT64_C(0xffffffffffff0000)
#define	TXN_SOFT_PAUSE_TIME			UINT64_C(0x000000000000ffff)

/* TX Time-to-Backpressure Registers */

#define	TXN_PAUSE_TOGO_XXX_63_16		UINT64_C(0xffffffffffff0000)
#define	TXN_PAUSE_TOGO_TIME			UINT64_C(0x000000000000ffff)

/* TX Pause-Zero-Enable Registers */

#define	TXN_PAUSE_ZERO_XXX_63_1			UINT64_C(0xfffffffffffffffe)
#define	TXN_PAUSE_ZERO_SEND			UINT64_C(0x0000000000000001)

/* GMX TX Statistics Control Registers */

#define	TXN_STATS_CTL_XXX_63_1			UINT64_C(0xfffffffffffffffe)
#define	TXN_STATS_CTL_RD_CLR			UINT64_C(0x0000000000000001)

/* GMX TX Transmit Control Registers */

#define	TXN_CTL_XXX_63_2			UINT64_C(0xfffffffffffffffc)
#define	TXN_CTL_XSDEF_EN			UINT64_C(0x0000000000000002)
#define	TXN_CTL_XSCOL_EN			UINT64_C(0x0000000000000001)

/* Transmit Statistics Registers 0 */

#define	TXN_STAT0_XSDEF				UINT64_C(0xffffffff00000000)
#define	TXN_STAT0_XSCOL				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 1 */

#define	TXN_STAT1_SCOL				UINT64_C(0xffffffff00000000)
#define	TXN_STAT1_MSCOL				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 2 */

#define	TXN_STAT2_XXX_63_48			UINT64_C(0xffff000000000000)
#define	TXN_STAT2_OCTS				UINT64_C(0x0000ffffffffffff)

/* Transmit Statistics Registers 3 */

#define	TXN_STAT3_XXX_63_48			UINT64_C(0xffffffff00000000)
#define	TXN_STAT3_PKTS				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 4 */

#define	TXN_STAT4_HIST1				UINT64_C(0xffffffff00000000)
#define	TXN_STAT4_HIST0				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 5 */

#define	TXN_STAT5_HIST3				UINT64_C(0xffffffff00000000)
#define	TXN_STAT5_HIST2				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 6 */

#define	TXN_STAT6_HIST5				UINT64_C(0xffffffff00000000)
#define	TXN_STAT6_HIST4				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 7 */

#define	TXN_STAT7_HIST7				UINT64_C(0xffffffff00000000)
#define	TXN_STAT7_HIST6				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Registers 8 */

#define	TXN_STAT8_MCST				UINT64_C(0xffffffff00000000)
#define	TXN_STAT8_BCST				UINT64_C(0x00000000ffffffff)

/* Transmit Statistics Register 9 */

#define	TXN_STAT9_UNDFLW			UINT64_C(0xffffffff00000000)
#define	TXN_STAT9_CTL				UINT64_C(0x00000000ffffffff)

/* BMX BIST Results Register */

#define	BIST_XXX_63_10				UINT64_C(0xfffffffffffffc00)
#define	BIST_STATUS				UINT64_C(0x00000000000003ff)

/* RX Ports Register */

#define	RX_PRTS_XXX_63_3			UINT64_C(0xfffffffffffffff8)
#define	RX_PRTS_PRTS				UINT64_C(0x0000000000000007)

/* RX FIFO Packet-Drop Registers */

#define	RX_BP_DROPN_XXX_63_6			UINT64_C(0xffffffffffffffc0)
#define	RX_BP_DROPN_MARK			UINT64_C(0x000000000000003f)

/* RX Backpressure On Registers */

#define	RX_BP_ONN_XXX_63_9			UINT64_C(0xfffffffffffffe00)
#define	RX_BP_ONN_MARK				UINT64_C(0x00000000000001ff)

/* RX Backpressure Off Registers */

#define	RX_BP_OFFN_XXX_63_6			UINT64_C(0xffffffffffffffc0)
#define	RX_BP_OFFN_MARK				UINT64_C(0x000000000000003f)

/* TX Ports Register */

#define	TX_PRTS_XXX_63_5			UINT64_C(0xffffffffffffffe0)
#define	TX_PRTS_PRTS				UINT64_C(0x000000000000001f)

/* TX Interframe Gap Register */

#define	TX_IFG_XXX_63_8				UINT64_C(0xffffffffffffff00)
#define	TX_IFG_IFG2				UINT64_C(0x00000000000000f0)
#define	TX_IFG_IFG1				UINT64_C(0x000000000000000f)

/* TX Jam Pattern Register */

#define	TX_JAM_XXX_63_8				UINT64_C(0xffffffffffffff00)
#define	TX_JAM_JAM				UINT64_C(0x00000000000000ff)

/* TX Collision Attempts Before Dropping Frame Register */

#define	TX_COL_ATTEMPT_XXX_63_5			UINT64_C(0xffffffffffffffe0)
#define	TX_COL_ATTEMPT_LIMIT			UINT64_C(0x000000000000001f)

/* TX Pause-Packet DMAC-Field Register */

#define	TX_PAUSE_PKT_DMAC_XXX_63_48		UINT64_C(0xffff000000000000)
#define	TX_PAUSE_PKT_DMAC_DMAC			UINT64_C(0x0000ffffffffffff)

/* TX Pause Packet Type Field Register */

#define	TX_PAUSE_PKT_TYPE_XXX_63_16		UINT64_C(0xffffffffffff0000)
#define	TX_PAUSE_PKT_TYPE_TYPE			UINT64_C(0x000000000000ffff)

/* TX Override Backpressure Register */

#define	TX_OVR_BP_XXX_63_12			UINT64_C(0xfffffffffffff000)
#define	TX_OVR_BP_XXX_11			UINT64_C(0x0000000000000800)
#define	TX_OVR_BP_EN				UINT64_C(0x0000000000000700)
#define	 TX_OVR_BP_EN_SHIFT			8
#define	TX_OVR_BP_XXX_7				UINT64_C(0x0000000000000080)
#define	TX_OVR_BP_BP				UINT64_C(0x0000000000000070)
#define	 TX_OVR_BP_BP_SHIFT			4
#define	TX_OVR_BP_XXX_3				UINT64_C(0x0000000000000008)
#define	TX_OVR_BP_IGN_FULL			UINT64_C(0x0000000000000007)
#define	 TX_OVR_BP_IGN_FULL_SHIFT		0

/* TX Override Backpressure Register */

#define	TX_OVR_BP_XXX_63_12			UINT64_C(0xfffffffffffff000)
#define	TX_OVR_BP_XXX_11			UINT64_C(0x0000000000000800)
#define	TX_OVR_BP_EN				UINT64_C(0x0000000000000700)
#define	TX_OVR_BP_XXX_7				UINT64_C(0x0000000000000080)
#define	TX_OVR_BP_BP				UINT64_C(0x0000000000000070)
#define	TX_OVR_BP_XXX_3				UINT64_C(0x0000000000000008)
#define	TX_OVR_BP_IGN_FULL			UINT64_C(0x0000000000000007)

/* TX Backpressure Status Register */

#define	TX_BP_SR_XXX_63_3			UINT64_C(0xfffffffffffffff8)
#define	TX_BP_SR_BP				UINT64_C(0x0000000000000007)

/* TX Corrupt Packets Register */

#define	TX_CORRUPT_XXX_63_3			UINT64_C(0xfffffffffffffff8)
#define	TX_CORRUPT_CORRUPT			UINT64_C(0x0000000000000007)

/* RX Port State Information Register */

#define	RX_PRT_INFO_XXX_63_19			UINT64_C(0xfffffffffff80000)
#define	RX_PRT_INFO_DROP			UINT64_C(0x0000000000070000)
#define	RX_PRT_INFO_XXX_15_3			UINT64_C(0x000000000000fff8)
#define	RX_PRT_INFO_COMMIT			UINT64_C(0x0000000000000007)

/* TX LFSR Register */

#define	TX_LFSR_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	TX_LFSR_LFSR				UINT64_C(0x000000000000ffff)

/* TX Interrupt Register */

#define	TX_INT_REG_XXX_63_20			UINT64_C(0xfffffffffff00000)
#define	TX_INT_REG_XXX_19			UINT64_C(0x0000000000080000)
#define	TX_INT_REG_LATE_COL			UINT64_C(0x0000000000070000)
#define	TX_INT_REG_XXX_15			UINT64_C(0x0000000000008000)
#define	TX_INT_REG_XSDEF			UINT64_C(0x0000000000007000)
#define	TX_INT_REG_XXX_11			UINT64_C(0x0000000000000800)
#define	TX_INT_REG_XSCOL			UINT64_C(0x0000000000000700)
#define	TX_INT_REG_XXX_7_5			UINT64_C(0x00000000000000e0)
#define	TX_INT_REG_UNDFLW			UINT64_C(0x000000000000001c)
#define	TX_INT_REG_XXX_1			UINT64_C(0x0000000000000002)
#define	TX_INT_REG_PKO_NXA			UINT64_C(0x0000000000000001)

/* TX Interrupt Register */

#define	TX_INT_EN_XXX_63_20			UINT64_C(0xfffffffffff00000)
#define	TX_INT_EN_XXX_19			UINT64_C(0x0000000000080000)
#define	TX_INT_EN_LATE_COL			UINT64_C(0x0000000000070000)
#define	TX_INT_EN_XXX_15			UINT64_C(0x0000000000008000)
#define	TX_INT_EN_XSDEF				UINT64_C(0x0000000000007000)
#define	TX_INT_EN_XXX_11			UINT64_C(0x0000000000000800)
#define	TX_INT_EN_XSCOL				UINT64_C(0x0000000000000700)
#define	TX_INT_EN_XXX_7_5			UINT64_C(0x00000000000000e0)
#define	TX_INT_EN_UNDFLW			UINT64_C(0x000000000000001c)
#define	TX_INT_EN_XXX_1				UINT64_C(0x0000000000000002)
#define	TX_INT_EN_PKO_NXA			UINT64_C(0x0000000000000001)

/* Address-out-of-Range Error Register */

#define	NXA_ADR_XXX_63_6			UINT64_C(0xffffffffffffffc0)
#define	NXA_ADR_PRT				UINT64_C(0x000000000000003f)

/* GMX Miscellaneous Error Register */

#define	BAD_REG_XXX_63_31			UINT64_C(0xffffffff80000000)
#define	BAD_REG_INB_NXA				UINT64_C(0x0000000078000000)
#define	BAD_REG_STATOVR				UINT64_C(0x0000000004000000)
#define	BAD_REG_XXX_25				UINT64_C(0x0000000002000000)
#define	BAD_REG_LOSTSTAT			UINT64_C(0x0000000001c00000)
#define	BAD_REG_XXX_21_18			UINT64_C(0x00000000003c0000)
#define	BAD_REG_XXX_17_5			UINT64_C(0x000000000003ffe0)
#define	BAD_REG_OUT_OVR				UINT64_C(0x000000000000001c)
#define	BAD_REG_XXX_1_0				UINT64_C(0x0000000000000003)

/* GMX Backpressure Statistics Register */

#define	STAT_BP_XXX_63_17			UINT64_C(0xfffffffffffe0000)
#define	STAT_BP_BP				UINT64_C(0x0000000000010000)
#define	STAT_BP_CNT				UINT64_C(0x000000000000ffff)

/* Mode Change Mask Registers */

#define	TX_CLK_MSKN_XXX_63_1			UINT64_C(0xfffffffffffffffe)
#define	TX_CLK_MSKN_MSK				UINT64_C(0x0000000000000001)

/* GMX RX/TX Status Register */

#define	RX_TX_STATUS_XXX_63_7			UINT64_C(0xffffffffffffff80)
#define	RX_TX_STATUS_TX				UINT64_C(0x0000000000000070)
#define	RX_TX_STATUS_XXX_3			UINT64_C(0x0000000000000008)
#define	RX_TX_STATUS_RX				UINT64_C(0x0000000000000007)

/* Interface Mode Register */

#define	INF_MODE_XXX_63_3			UINT64_C(0xfffffffffffffff8)
#define	INF_MODE_P0MII				UINT64_C(0x0000000000000004)
#define	INF_MODE_EN				UINT64_C(0x0000000000000002)
#define	INF_MODE_TYPE				UINT64_C(0x0000000000000001)

/* -------------------------------------------------------------------------- */

/* for bus_space(9) */

#define GMX_IF_NUNITS				1
#define GMX_PORT_NUNITS				3

#define	GMX0_BASE_PORT0				0x0001180008000000ULL
#define	GMX0_BASE_PORT1				0x0001180008000800ULL
#define	GMX0_BASE_PORT2				0x0001180008001000ULL
#define	GMX0_BASE_PORT_SIZE				0x00800
#define	GMX0_BASE_IF0				0x0001180008000000ULL
#define	GMX0_BASE_IF_SIZE			(GMX0_BASE_PORT_SIZE * GMX_PORT_NUNITS)

/* for snprintb(9) */

#define	RXN_INT_REG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x12"		"PHY_DUPX\0" \
	"b\x11"		"PHY_SPD\0" \
	"b\x10"		"PHY_LINK\0" \
	"b\x0f"		"IFGERR\0" \
	"b\x0e"		"COLDET\0" \
	"b\x0d"		"FALERR\0" \
	"b\x0c"		"RSVERR\0" \
	"b\x0b"		"PCTERR\0" \
	"b\x0a"		"OVRERR\0" \
	"b\x09"		"NIBERR\0" \
	"b\x08"		"SKPERR\0" \
	"b\x07"		"RCVERR\0" \
	"b\x06"		"LENERR\0" \
	"b\x05"		"ALNERR\0" \
	"b\x04"		"FCSERR\0" \
	"b\x03"		"JABBER\0" \
	"b\x02"		"MAXERR\0" \
	"b\x01"		"CAREXT\0" \
	"b\x00"		"MINERR\0"
#define	RXN_INT_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x12"		"PHY_DUPX\0" \
	"b\x11"		"PHY_SPD\0" \
	"b\x10"		"PHY_LINK\0" \
	"b\x0f"		"IFGERR\0" \
	"b\x0e"		"COLDET\0" \
	"b\x0d"		"FALERR\0" \
	"b\x0c"		"RSVERR\0" \
	"b\x0b"		"PCTERR\0" \
	"b\x0a"		"OVRERR\0" \
	"b\x09"		"NIBERR\0" \
	"b\x08"		"SKPERR\0" \
	"b\x07"		"RCVERR\0" \
	"b\x06"		"LENERR\0" \
	"b\x05"		"ALNERR\0" \
	"b\x04"		"FCSERR\0" \
	"b\x03"		"JABBER\0" \
	"b\x02"		"MAXERR\0" \
	"b\x01"		"CAREXT\0" \
	"b\x00"		"MINERR\0"
#define	PRTN_CFG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x03"		"SLOTTIME\0" \
	"b\x02"		"DUPLEX\0" \
	"b\x01"		"SPEED\0" \
	"b\x00"		"EN\0"
#define	RXN_FRM_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x0a"		"NULL_DIS\0" \
	"b\x09"		"PRE_ALIGN\0" \
	"b\x08"		"PAD_LEN\0" \
	"b\x07"		"VLAN_LEN\0" \
	"b\x06"		"PRE_FREE\0" \
	"b\x05"		"CTL_SMAC\0" \
	"b\x04"		"CTL_MCST\0" \
	"b\x03"		"CTL_BCK\0" \
	"b\x02"		"CTL_DRP\0" \
	"b\x01"		"PRE_STRP\0" \
	"b\x00"		"PRE_CHK\0"
#define	RXN_FRM_CHK_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x09"		"NIBERR\0" \
	"b\x08"		"SKPERR\0" \
	"b\x07"		"RCVERR\0" \
	"b\x06"		"LENERR\0" \
	"b\x05"		"ALNERR\0" \
	"b\x04"		"FCSERR\0" \
	"b\x03"		"JABBER\0" \
	"b\x02"		"MAXERR\0" \
	"b\x01"		"CAREXT\0" \
	"b\x00"		"MINERR\0"
/* RXN_FRM_MIN */
/* RXN_FRM_MAX */
#define	RXN_JABBER_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"CNT\0"
#define	RXN_DECISION_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x05"	"CNT\0"
#define	RXN_UDD_SKP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x08"		"FCSSEL\0" \
	"f\x00\x07"	"LEN\0"
#define	RXN_STATS_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x00"		"RD_CLR\0"
#define	RXN_IFG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x04"	"IFG\0"
#define	RXN_RX_INBND_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x03"		"DUPLEX\0" \
	"f\x01\x02"	"SPEED\0" \
	"b\x00"		"STATUS\0"
#define	RXN_STATS_PKTS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"CNT\0"
#define	RXN_STATS_OCTS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"CNT\0"
#define	RXN_STATS_PKTS_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"CNT\0"
#define	RXN_STATS_OCTS_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"CNT\0"
#define	RXN_STATS_PKTS_DMAC_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"CNT\0"
#define	RXN_STATS_OCTS_DMAC_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"CNT\0"
#define	RXN_STATS_PKTS_DRP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"CNT\0"
#define	RXN_STATS_OCTS_DRP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"CNT\0"
#define	RXN_STATS_PKTS_BAD_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"CNT\0"
#define	RXN_ADR_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x03"		"CAM_MODE\0" \
	"f\x01\x02"	"MCST\0" \
	"b\x00"		"BCST\0"
#define	RXN_ADR_CAM_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x08"	"EN\0"
/* RXN_ADR_CAM0 */
/* RXN_ADR_CAM1 */
/* RXN_ADR_CAM2 */
/* RXN_ADR_CAM3 */
/* RXN_ADR_CAM4 */
/* RXN_ADR_CAM5 */
#define	TXN_CLK_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x06"	"CLK_CNT\0"
#define	TXN_THRESH_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x06"	"CNT\0"
#define	TXN_APPEND_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x03"		"FORCE_FCS\0" \
	"b\x02"		"FCS\0" \
	"b\x01"		"PAD\0" \
	"b\x00"		"PREAMBLE\0"
#define	TXN_SLOT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x0a"	"SLOT\0"
#define	TXN_BURST_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"BURST\0"
/* SMAC0 */
#define	TXN_PAUSE_PKT_TIME_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"TIME\0"
#define	TXN_MIN_PKT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x08"	"MIN_SIZE\0"
#define	TXN_PAUSE_PKT_INTERVAL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"INTERVAL\0"
#define	TXN_SOFT_PAUSE_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"TIME\0"
#define	TXN_PAUSE_TOGO_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"TIME\0"
#define	TXN_PAUSE_ZERO_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x00"		"SEND\0"
#define	TXN_STATS_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x00"		"RD_CLR\0"
#define	TXN_CTL_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x01"		"XSDEF_EN\0" \
	"b\x00"		"XSCOL_EN\0"
#define	TXN_STAT0_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"XSDEF\0" \
	"f\x00\x20"	"XSCOL\0"
#define	TXN_STAT1_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"SCOL\0" \
	"f\x00\x20"	"MSCOL\0"
#define	TXN_STAT2_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"OCTS\0"
#define	TXN_STAT3_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x20"	"PKTS\0"
#define	TXN_STAT4_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"HIST1\0" \
	"f\x00\x20"	"HIST0\0"
#define	TXN_STAT5_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"HIST3\0" \
	"f\x00\x20"	"HIST2\0"
#define	TXN_STAT6_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"HIST5\0" \
	"f\x00\x20"	"HIST4\0"
#define	TXN_STAT7_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"HIST7\0" \
	"f\x00\x20"	"HIST6\0"
#define	TXN_STAT8_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"MCST\0" \
	"f\x00\x20"	"BCST\0"
#define	TXN_STAT9_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x20\x20"	"UNDFLW\0" \
	"f\x00\x20"	"CTL\0"
/* BIST0 */
#define	RX_PRTS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x03"	"PRTS\0"
#define	RX_BP_DROPN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x06"	"MARK\0"
#define	RX_BP_ONN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x09"	"MARK\0"
#define	RX_BP_OFFN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x06"	"MARK\0"
#define	TX_PRTS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x05"	"PRTS\0"
#define	TX_IFG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x04\x04"	"IFG2\0" \
	"f\x00\x04"	"IFG1\0"
#define	TX_JAM_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x08"	"JAM\0"
#define	TX_COL_ATTEMPT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x05"	"LIMIT\0"
#define	TX_PAUSE_PKT_DMAC_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x30"	"DMAC\0"
#define	TX_PAUSE_PKT_TYPE_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"TYPE\0"
#define	TX_OVR_BP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x08\x03"	"EN\0" \
	"f\x04\x03"	"BP\0" \
	"f\x00\x03"	"IGN_FULL\0"
#define	TX_BP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x03"	"SR_BP\0"
#define	TX_CORRUPT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x03"	"CORRUPT\0"
#define	RX_PRT_INFO_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x10\x03"	"DROP\0" \
	"f\x00\x03"	"COMMIT\0"
#define	TX_LFSR_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"LFSR\0"
#define	TX_INT_REG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x10\x03"	"LATE_COL\0" \
	"f\x0c\x03"	"XSDEF\0" \
	"f\x08\x03"	"XSCOL\0" \
	"f\x02\x03"	"UNDFLW\0" \
	"b\x00"		"PKO_NXA\0"
#define	TX_INT_EN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x10\x03"	"LATE_COL\0" \
	"f\x0c\x03"	"XSDEF\0" \
	"f\x08\x03"	"XSCOL\0" \
	"f\x02\x03"	"UNDFLW\0" \
	"b\x00"		"PKO_NXA\0"
#define	NXA_ADR_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x06"	"PRT\0"
#define	BAD_REG_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x1b\x04"	"INB_NXA\0" \
	"b\x1a"		"STATOVR\0" \
	"f\x16\x03"	"LOSTSTAT\0" \
	"f\x02\x03"	"OUT_OVR\0"
#define	STAT_BP_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x10"		"BP\0" \
	"f\x00\x10"	"CNT\0"
#define	TX_CLK_MSKN_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x00"		"MSK\0"
#define	RX_TX_STATUS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x04\x03"	"TX\0" \
	"f\x00\x03"	"RX\0"
#define	INF_MODE_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x02"		"P0MII\0" \
	"b\x01"		"EN\0" \
	"b\x00"		"TYPE\0"

#define GMX0_RX0_INT_REG_BITS			RXN_INT_REG_BITS
#define GMX0_RX0_INT_EN_BITS			RXN_INT_EN_BITS
#define GMX0_PRT0_CFG_BITS			PRTN_CFG_BITS
#define GMX0_RX0_FRM_CTL_BITS			RXN_FRM_CTL_BITS
#define GMX0_RX0_FRM_CHK_BITS			RXN_FRM_CHK_BITS
#define GMX0_RX0_FRM_MIN_BITS			NULL//RXN_FRM_MIN_BITS
#define GMX0_RX0_FRM_MAX_BITS			NULL//RXN_FRM_MAX_BITS
#define GMX0_RX0_JABBER_BITS			RXN_JABBER_BITS
#define GMX0_RX0_DECISION_BITS			RXN_DECISION_BITS
#define GMX0_RX0_UDD_SKP_BITS			RXN_UDD_SKP_BITS
#define GMX0_RX0_STATS_CTL_BITS			RXN_STATS_CTL_BITS
#define GMX0_RX0_IFG_BITS			RXN_IFG_BITS
#define GMX0_RX0_RX_INBND_BITS			RXN_RX_INBND_BITS
#define GMX0_RX0_STATS_PKTS_BITS		RXN_STATS_PKTS_BITS
#define GMX0_RX0_STATS_OCTS_BITS		RXN_STATS_OCTS_BITS
#define GMX0_RX0_STATS_PKTS_CTL_BITS		RXN_STATS_PKTS_CTL_BITS
#define GMX0_RX0_STATS_OCTS_CTL_BITS		RXN_STATS_OCTS_CTL_BITS
#define GMX0_RX0_STATS_PKTS_DMAC_BITS		RXN_STATS_PKTS_DMAC_BITS
#define GMX0_RX0_STATS_OCTS_DMAC_BITS		RXN_STATS_OCTS_DMAC_BITS
#define GMX0_RX0_STATS_PKTS_DRP_BITS		RXN_STATS_PKTS_DRP_BITS
#define GMX0_RX0_STATS_OCTS_DRP_BITS		RXN_STATS_OCTS_DRP_BITS
#define GMX0_RX0_STATS_PKTS_BAD_BITS		RXN_STATS_PKTS_BAD_BITS
#define GMX0_RX0_ADR_CTL_BITS			RXN_ADR_CTL_BITS
#define GMX0_RX0_ADR_CAM_EN_BITS		RXN_ADR_CAM_EN_BITS
#define GMX0_RX0_ADR_CAM0_BITS			NULL//RXN_ADR_CAM0_BITS
#define GMX0_RX0_ADR_CAM1_BITS			NULL//RXN_ADR_CAM1_BITS
#define GMX0_RX0_ADR_CAM2_BITS			NULL//RXN_ADR_CAM2_BITS
#define GMX0_RX0_ADR_CAM3_BITS			NULL//RXN_ADR_CAM3_BITS
#define GMX0_RX0_ADR_CAM4_BITS			NULL//RXN_ADR_CAM4_BITS
#define GMX0_RX0_ADR_CAM5_BITS			NULL//RXN_ADR_CAM5_BITS
#define GMX0_TX0_CLK_BITS			TXN_CLK_BITS
#define GMX0_TX0_THRESH_BITS			TXN_THRESH_BITS
#define GMX0_TX0_APPEND_BITS			TXN_APPEND_BITS
#define GMX0_TX0_SLOT_BITS			TXN_SLOT_BITS
#define GMX0_TX0_BURST_BITS			TXN_BURST_BITS
#define GMX0_SMAC0_BITS				NULL//SMAC0_BITS
#define GMX0_TX0_PAUSE_PKT_TIME_BITS		TXN_PAUSE_PKT_TIME_BITS
#define GMX0_TX0_MIN_PKT_BITS			TXN_MIN_PKT_BITS
#define GMX0_TX0_PAUSE_PKT_INTERVAL_BITS	TXN_PAUSE_PKT_INTERVAL_BITS
#define GMX0_TX0_SOFT_PAUSE_BITS		TXN_SOFT_PAUSE_BITS
#define GMX0_TX0_PAUSE_TOGO_BITS		TXN_PAUSE_TOGO_BITS
#define GMX0_TX0_PAUSE_ZERO_BITS		TXN_PAUSE_ZERO_BITS
#define GMX0_TX0_STATS_CTL_BITS			TXN_STATS_CTL_BITS
#define GMX0_TX0_CTL_BITS			TXN_CTL_BITS
#define GMX0_TX0_STAT0_BITS			TXN_STAT0_BITS
#define GMX0_TX0_STAT1_BITS			TXN_STAT1_BITS
#define GMX0_TX0_STAT2_BITS			TXN_STAT2_BITS
#define GMX0_TX0_STAT3_BITS			TXN_STAT3_BITS
#define GMX0_TX0_STAT4_BITS			TXN_STAT4_BITS
#define GMX0_TX0_STAT5_BITS			TXN_STAT5_BITS
#define GMX0_TX0_STAT6_BITS			TXN_STAT6_BITS
#define GMX0_TX0_STAT7_BITS			TXN_STAT7_BITS
#define GMX0_TX0_STAT8_BITS			TXN_STAT8_BITS
#define GMX0_TX0_STAT9_BITS			TXN_STAT9_BITS
#define GMX0_BIST0_BITS				NULL//BIST0_BITS
#define GMX0_RX_PRTS_BITS			RX_PRTS_BITS
#define GMX0_RX_BP_DROP0_BITS			RX_BP_DROPN_BITS
#define GMX0_RX_BP_ON0_BITS			RX_BP_ONN_BITS
#define GMX0_RX_BP_OFF0_BITS			RX_BP_OFFN_BITS
#define GMX0_RX_BP_DROP1_BITS			RX_BP_DROPN_BITS
#define GMX0_RX_BP_ON1_BITS			RX_BP_ONN_BITS
#define GMX0_RX_BP_OFF1_BITS			RX_BP_OFFN_BITS
#define GMX0_RX_BP_DROP2_BITS			RX_BP_DROPN_BITS
#define GMX0_RX_BP_ON2_BITS			RX_BP_ONN_BITS
#define GMX0_RX_BP_OFF2_BITS			RX_BP_OFFN_BITS
#define GMX0_TX_PRTS_BITS			TX_PRTS_BITS
#define GMX0_TX_IFG_BITS			TX_IFG_BITS
#define GMX0_TX_JAM_BITS			TX_JAM_BITS
#define GMX0_TX_COL_ATTEMPT_BITS		TX_COL_ATTEMPT_BITS
#define GMX0_TX_PAUSE_PKT_DMAC_BITS		TX_PAUSE_PKT_DMAC_BITS
#define GMX0_TX_PAUSE_PKT_TYPE_BITS		TX_PAUSE_PKT_TYPE_BITS
#define GMX0_TX_OVR_BP_BITS			TX_OVR_BP_BITS
#define GMX0_TX_BP_BITS				TX_BP_BITS
#define GMX0_TX_CORRUPT_BITS			TX_CORRUPT_BITS
#define GMX0_RX_PRT_INFO_BITS			RX_PRT_INFO_BITS
#define GMX0_TX_LFSR_BITS			TX_LFSR_BITS
#define GMX0_TX_INT_REG_BITS			TX_INT_REG_BITS
#define GMX0_TX_INT_EN_BITS			TX_INT_EN_BITS
#define GMX0_NXA_ADR_BITS			NXA_ADR_BITS
#define GMX0_BAD_REG_BITS			BAD_REG_BITS
#define GMX0_STAT_BP_BITS			STAT_BP_BITS
#define GMX0_TX_CLK_MSK0_BITS			TX_CLK_MSKN_BITS
#define GMX0_TX_CLK_MSK1_BITS			TX_CLK_MSKN_BITS
#define GMX0_TX_CLK_MSK2_BITS			TX_CLK_MSKN_BITS
#define GMX0_RX_TX_STATUS_BITS			RX_TX_STATUS_BITS
#define GMX0_INF_MODE_BITS			INF_MODE_BITS

#endif /* _OCTEON_GMXREG_H_ */
