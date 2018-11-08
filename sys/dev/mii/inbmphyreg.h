/*	$NetBSD: inbmphyreg.h,v 1.9.8.2 2018/11/08 12:09:18 martin Exp $	*/
/*******************************************************************************
Copyright (c) 2001-2005, Intel Corporation 
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
 * 15-5: page
 * 4-0: register offset
 */
#define BME1000_PAGE_SHIFT        5
#define BME1000_REG(page, reg)    \
        (((page) << BME1000_PAGE_SHIFT) | ((reg) & MII_ADDRMASK))

#define BME1000_MAX_MULTI_PAGE_REG     0xf   /* Registers equal on all pages */

#define	BM_PHY_REG_PAGE(offset)			\
	((uint16_t)(((offset) >> BME1000_PAGE_SHIFT) & 0xffff))
#define	BM_PHY_REG_NUM(offset)				\
	((uint16_t)((offset) & MII_ADDRMASK)		\
	| (((offset) >> (21 - BME1000_PAGE_SHIFT)) & ~MII_ADDRMASK))

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

#define HV_LED_CONFIG		BME1000_REG(0, 30)

#define	HV_KMRN_MODE_CTRL	BME1000_REG(BM_PORT_CTRL_PAGE, 16)
#define	HV_KMRN_MDIO_SLOW	0x0400

#define	BM_PORT_GEN_CFG		BME1000_REG(BM_PORT_CTRL_PAGE, 17)

#define	CV_SMB_CTRL		BME1000_REG(BM_PORT_CTRL_PAGE, 23)
#define	CV_SMB_CTRL_FORCE_SMBUS	__BIT(0)

#define	HV_PM_CTRL		BME1000_REG(770, 17)
#define	HV_PM_CTRL_K1_ENA	__BIT(14)

#define	I217_INBAND_CTRL	BME1000_REG(770, 18)
#define	I217_INBAND_CTRL_LINK_STAT_TX_TIMEOUT_MASK	0x3f00
#define	I217_INBAND_CTRL_LINK_STAT_TX_TIMEOUT_SHIFT	8

#define	IGP3_KMRN_DIAG		BME1000_REG(770, 19)
#define	IGP3_KMRN_DIAG_PCS_LOCK_LOSS	(1 << 1)

#define HV_MUX_DATA_CTRL	BME1000_REG(776, 16)
#define HV_MUX_DATA_CTRL_FORCE_SPEED	(1 << 2)
#define HV_MUX_DATA_CTRL_GEN_TO_MAC	(1 << 10)

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
#define	BM_WUC			BME1000_REG(BM_WUC_PAGE, 1)
#define	BM_WUC_ADDRESS_OPCODE	0x11
#define	BM_WUC_DATA_OPCODE	0x12
#define	BM_WUC_ENABLE_PAGE	BM_PORT_CTRL_PAGE
#define	BM_WUC_ENABLE_REG	17
#define	BM_WUC_ENABLE_BIT	(1 << 2)
#define	BM_WUC_HOST_WU_BIT	(1 << 4)
#define	BM_WUC_ME_WU_BIT	(1 << 5)

#endif /* _DEV_MII_INBMPHYREG_H_ */
