/*	$NetBSD: inbmphyreg.h,v 1.18.4.1 2020/09/23 08:46:54 martin Exp $	*/
/*******************************************************************************
Copyright (c) 2001-2015, Intel Corporation 
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the Intel Corporation nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/*
 * Copied from the Intel code, and then modified to match NetBSD
 * style for MII registers more.
 */

#ifndef _DEV_MII_INBMPHYREG_H_
#define	_DEV_MII_INBMPHYREG_H_

/* Bits...
 * 31-16: register offset (high)
 * 15-5:  page
 * 4-0:   register offset (low)
 */
#define BME1000_PAGE_SHIFT	5
#define BM_PHY_UPPER_SHIFT	21
#define BME1000_REG(page, reg)    \
        (((reg) & MII_ADDRMASK) | 			\
	    (((page) & 0xffff) << BME1000_PAGE_SHIFT) |	\
	    (((reg) & ~MII_ADDRMASK) << (BM_PHY_UPPER_SHIFT - BME1000_PAGE_SHIFT)))

#define BME1000_MAX_MULTI_PAGE_REG     0xf   /* Registers equal on all pages */

#define	BM_PHY_REG_PAGE(offset)			\
	((uint16_t)(((offset) >> BME1000_PAGE_SHIFT) & 0xffff))
#define	BM_PHY_REG_NUM(offset)				\
	((uint16_t)((offset) & MII_ADDRMASK)		\
	| (((offset) >> (BM_PHY_UPPER_SHIFT - BME1000_PAGE_SHIFT)) & ~MII_ADDRMASK))

/* BME1000 Specific Registers */
#define BME1000_PHY_SPEC_CTRL	BME1000_REG(0, 16) /* PHY Specific Control */
#define BME1000_PSCR_DISABLE_JABBER             0x0001 /* 1=Disable Jabber */
#define BME1000_PSCR_POLARITY_REVERSAL_DISABLE  0x0002 /* 1=Polarity Reversal Disabled */
#define BME1000_PSCR_POWER_DOWN                 0x0004 /* 1=Power Down */
#define BME1000_PSCR_COPPER_TRANSMITER_DISABLE  0x0008 /* 1=Transmitter Disabled */
#define BME1000_PSCR_CROSSOVER_MODE_MASK        0x0060
#define BME1000_PSCR_CROSSOVER_MODE_MDI         0x0000 /* 00=Manual MDI configuration */
#define BME1000_PSCR_CROSSOVER_MODE_MDIX        0x0020 /* 01=Manual MDIX configuration */
#define BME1000_PSCR_CROSSOVER_MODE_AUTO        0x0060 /* 11=Automatic crossover */
#define BME1000_PSCR_ENALBE_EXTENDED_DISTANCE   0x0080 /* 1=Enable Extended Distance */
#define BME1000_PSCR_ENERGY_DETECT_MASK         0x0300
#define BME1000_PSCR_ENERGY_DETECT_OFF          0x0000 /* 00,01=Off */
#define BME1000_PSCR_ENERGY_DETECT_RX           0x0200 /* 10=Sense on Rx only (Energy Detect) */
#define BME1000_PSCR_ENERGY_DETECT_RX_TM        0x0300 /* 11=Sense and Tx NLP */
#define BME1000_PSCR_FORCE_LINK_GOOD            0x0400 /* 1=Force Link Good */
#define BME1000_PSCR_DOWNSHIFT_ENABLE           0x0800 /* 1=Enable Downshift */
#define BME1000_PSCR_DOWNSHIFT_COUNTER_MASK     0x7000
#define BME1000_PSCR_DOWNSHIFT_COUNTER_SHIFT    12

/* Extended Management Interface (EMI) Registers */
#define I82579_EMI_ADDR	0x10
#define I82579_EMI_DATA	0x11
#define I82579_LPI_UPDATE_TIMER	0x4805 /* in 40ns units + 40 ns base value */
#define I82579_MSE_THRESHOLD	0x084F /* 82579 Mean Square Error Threshold */
#define I82577_MSE_THRESHOLD	0x0887 /* 82577 Mean Square Error Threshold */
#define I82579_MSE_LINK_DOWN	0x2411 /* MSE count before dropping link */
#define I82579_EEE_ADVERTISEMENT 0x040e  /* IEEE MMD Register 7.60 */
#define I82579_EEE_LP_ABILITY	0x040f   /* IEEE MMD Register 7.61 */
#define I82579_EEE_PCS_STATUS	0x182e
#define I82579_RX_CONFIG	0x3412 /* Receive configuration */
#define I82579_LPI_PLL_SHUT	0x4412
#define I82579_LPI_PLL_SHUT_100	__BIT(2) /* 100M LPI PLL Shut Enable */
#define I217_EEE_PCS_STATUS	0x9401   /* IEEE MMD Register 3.1 */
#define I217_EEE_CAPABILITY	0x8000   /* IEEE MMD Register 3.20 */
#define I217_EEE_ADVERTISEMENT	0x8001   /* IEEE MMD Register 7.60 */
#define I217_EEE_LP_ABILITY	0x8002   /* IEEE MMD Register 7.61 */
#define I217_RX_CONFIG		0xb20c   /* Receive configuration */

/* BM PHY Copper Specific Status */
#define BM_CS_STATUS		BME1000_REG(0, 17)
#define BM_CS_STATUS_LINK_UP	0x0400
#define BM_CS_STATUS_RESOLVED	0x0800
#define BM_CS_STATUS_SPEED_MASK	0xC000
#define BM_CS_STATUS_SPEED_1000	0x8000

#define BME1000_PHY_PAGE_SELECT	BME1000_REG(0, 22) /* Page Select */

#define BME1000_BIAS_SETTING	29
#define BME1000_BIAS_SETTING2	30

#define	I82578_ADDR_REG		29
#define	I82577_ADDR_REG		16
#define	I82577_CFG_REG		22

#define HV_INTC_FC_PAGE_START	768
#define	BM_PORT_CTRL_PAGE	769

#define HV_OEM_BITS		BME1000_REG(0, 25)
#define HV_OEM_BITS_LPLU	(1 << 2)
#define HV_OEM_BITS_A1KDIS	(1 << 6)
#define HV_OEM_BITS_ANEGNOW	(1 << 10)

/* 82577 Mobile Phy Status Register */
#define HV_M_STATUS		BME1000_REG(0, 26)
#define HV_M_STATUS_AUTONEG_COMPLETE 0x1000
#define HV_M_STATUS_SPEED_MASK	0x0300
#define HV_M_STATUS_SPEED_1000	0x0200
#define HV_M_STATUS_SPEED_100	0x0100
#define HV_M_STATUS_LINK_UP	0x0040

#define HV_LED_CONFIG		BME1000_REG(0, 30)

#define	HV_KMRN_MODE_CTRL	BME1000_REG(BM_PORT_CTRL_PAGE, 16)
#define	HV_KMRN_MDIO_SLOW	0x0400

#define	BM_PORT_GEN_CFG		BME1000_REG(BM_PORT_CTRL_PAGE, 17)

#define	I82579_DFT_CTRL		BME1000_REG(BM_PORT_CTRL_PAGE, 20)

#define	CV_SMB_CTRL		BME1000_REG(BM_PORT_CTRL_PAGE, 23)
#define	CV_SMB_CTRL_FORCE_SMBUS	__BIT(0)

#define	BM_RATE_ADAPTATION_CTRL	BME1000_REG(BM_PORT_CTRL_PAGE, 25)
#define	BM_RATE_ADAPTATION_CTRL_RX_RXDV_PRE	__BIT(8)
#define	BM_RATE_ADAPTATION_CTRL_RX_CRS_PRE	__BIT(7)

/* KMRN FIFO Control and Status */
#define HV_KMRN_FIFO_CTRLSTA			BME1000_REG(770, 16)
#define HV_KMRN_FIFO_CTRLSTA_PREAMBLE_MASK	0x7000
#define HV_KMRN_FIFO_CTRLSTA_PREAMBLE_SHIFT	12

#define	HV_PM_CTRL		BME1000_REG(770, 17)
#define HV_PM_CTRL_K1_CLK_REQ	__BIT(9)
#define	HV_PM_CTRL_K1_ENA	__BIT(14)

#define	I217_INBAND_CTRL	BME1000_REG(770, 18)
#define	I217_INBAND_CTRL_LINK_STAT_TX_TIMEOUT_MASK	0x3f00
#define	I217_INBAND_CTRL_LINK_STAT_TX_TIMEOUT_SHIFT	8

#define	IGP3_KMRN_DIAG		BME1000_REG(770, 19)
#define	IGP3_KMRN_DIAG_PCS_LOCK_LOSS	(1 << 1)

#define	I217_LPI_GPIO_CTRL	BME1000_REG(772, 18)
#define	I217_LPI_GPIO_CTRL_AUTO_EN_LPI	__BIT(11)

#define	I82579_LPI_CTRL		BME1000_REG(772, 20)
#define	I82579_LPI_CTRL_ENABLE	__BITS(14, 13)
#define	I82579_LPI_CTRL_EN_100	__BIT(13)
#define	I82579_LPI_CTRL_EN_1000	__BIT(14)

#define	I217_MEMPWR		BME1000_REG(772, 26)
#define	I217_MEMPWR_DISABLE_SMB_RELEASE		0x0010

#define I217_PLL_CLOCK_GATE_REG	BME1000_REG(772, 28)
#define I217_PLL_CLOCK_GATE_MASK	0x07FF

#define	I217_CFGREG		BME1000_REG(772, 29)
#define I217_CGFREG_ENABLE_MTA_RESET	0x0002

#define HV_MUX_DATA_CTRL	BME1000_REG(776, 16)
#define HV_MUX_DATA_CTRL_FORCE_SPEED	(1 << 2)
#define HV_MUX_DATA_CTRL_GEN_TO_MAC	(1 << 10)

#define I82579_UNKNOWN1		BME1000_REG(776, 20)
#define I82579_TX_PTR_GAP	0x1f

#define I218_ULP_CONFIG1	BME1000_REG(779, 16)
#define I218_ULP_CONFIG1_START		__BIT(0)
#define I218_ULP_CONFIG1_IND		__BIT(2)
#define I218_ULP_CONFIG1_STICKY_ULP	__BIT(4)
#define I218_ULP_CONFIG1_INBAND_EXIT	__BIT(5)
#define I218_ULP_CONFIG1_WOL_HOST	__BIT(6)
#define I218_ULP_CONFIG1_RESET_TO_SMBUS	__BIT(8)
#define I218_ULP_CONFIG1_EN_ULP_LANPHYPC __BIT(10)
#define I218_ULP_CONFIG1_DIS_CLR_STICKY_ON_PERST __BIT(11)
#define I218_ULP_CONFIG1_DIS_SMB_PERST	__BIT(12)

#define	BM_WUC_PAGE		800

#define	BM_RCTL			BME1000_REG(BM_WUC_PAGE, 0)
#define BM_RCTL_UPE		0x0001 /* Unicast Promiscuous Mode */
#define BM_RCTL_MPE		0x0002 /* Multicast Promiscuous Mode */
#define BM_RCTL_MO_SHIFT	3      /* Multicast Offset Shift */
#define BM_RCTL_MO_MASK		(3 << 3) /* Multicast Offset Mask */
#define BM_RCTL_BAM		0x0020 /* Broadcast Accept Mode */
#define BM_RCTL_PMCF		0x0040 /* Pass MAC Control Frames */
#define BM_RCTL_RFCE		0x0080 /* Rx Flow Control Enable */

#define	BM_WUC			BME1000_REG(BM_WUC_PAGE, 1)
#define	BM_WUC_ADDRESS_OPCODE	0x11
#define	BM_WUC_DATA_OPCODE	0x12
#define	BM_WUC_ENABLE_PAGE	BM_PORT_CTRL_PAGE
#define	BM_WUC_ENABLE_REG	17
#define	BM_WUC_ENABLE_BIT	(1 << 2)
#define	BM_WUC_HOST_WU_BIT	(1 << 4)
#define	BM_WUC_ME_WU_BIT	(1 << 5)

#define	BM_WUFC			BME1000_REG(BM_WUC_PAGE, 2)

#define	I217_PROXY_CTRL		BME1000_REG(BM_WUC_PAGE, 70)
#define I217_PROXY_CTRL_AUTO_DISABLE	0x0080

#define BM_RAR_L(_i)		(BME1000_REG(BM_WUC_PAGE, 16 + ((_i) << 2)))
#define BM_RAR_M(_i)		(BME1000_REG(BM_WUC_PAGE, 17 + ((_i) << 2)))
#define BM_RAR_H(_i)		(BME1000_REG(BM_WUC_PAGE, 18 + ((_i) << 2)))
#define BM_RAR_CTRL(_i)		(BME1000_REG(BM_WUC_PAGE, 19 + ((_i) << 2)))
#define BM_MTA(_i)		(BME1000_REG(BM_WUC_PAGE, 128 + ((_i) << 1)))

#endif /* _DEV_MII_INBMPHYREG_H_ */
