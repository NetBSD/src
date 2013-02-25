/* $NetBSD: omap3_i2creg.h,v 1.1.6.2 2013/02/25 00:28:31 tls Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _OMAP3_I2CREG_H
#define _OMAP3_I2CREG_H

/*
 * High-Speed I2C Controller registers
 */

#define OMAP3_I2C_REV		0x00	/* IP revision code */
#define  OMAP3_I2C_REV_MAJ_MASK		__BITS(4,7)
#define  OMAP3_I2C_REV_MAJ_SHIFT	4
#define  OMAP3_I2C_REV_MIN_MASK		__BITS(0,3)
#define  OMAP3_I2C_REV_MIN_SHIFT	0

#define OMAP3_I2C_IE		0x04	/* Interrupt enable */
#define  OMAP3_I2C_IE_XDR_IE		__BIT(14)
#define  OMAP3_I2C_IE_RDR_IE		__BIT(13)
#define  OMAP3_I2C_IE_AAS_IE		__BIT(9)
#define  OMAP3_I2C_IE_BF_IE		__BIT(8)
#define  OMAP3_I2C_IE_AERR_IE		__BIT(7)
#define  OMAP3_I2C_IE_STC_IE		__BIT(6)
#define  OMAP3_I2C_IE_GC_IE		__BIT(5)
#define  OMAP3_I2C_IE_XRDY_IE		__BIT(4)
#define  OMAP3_I2C_IE_RRDY_IE		__BIT(3)
#define  OMAP3_I2C_IE_ARDY_IE		__BIT(2)
#define  OMAP3_I2C_IE_NACK_IE		__BIT(1)
#define  OMAP3_I2C_IE_AL_IE		__BIT(0)

#define OMAP3_I2C_STAT		0x08	/* Interrupt status */
#define  OMAP3_I2C_STAT_XDR		__BIT(14)
#define  OMAP3_I2C_STAT_RDR		__BIT(13)
#define  OMAP3_I2C_STAT_BB		__BIT(12)
#define  OMAP3_I2C_STAT_ROVR		__BIT(11)
#define  OMAP3_I2C_STAT_XUDF		__BIT(10)
#define  OMAP3_I2C_STAT_AAS		__BIT(9)
#define  OMAP3_I2C_STAT_BF		__BIT(8)
#define  OMAP3_I2C_STAT_AERR		__BIT(7)
#define  OMAP3_I2C_STAT_STC		__BIT(6)
#define  OMAP3_I2C_STAT_GC		__BIT(5)
#define  OMAP3_I2C_STAT_XRDY		__BIT(4)
#define  OMAP3_I2C_STAT_RRDY		__BIT(3)
#define  OMAP3_I2C_STAT_ARDY		__BIT(2)
#define  OMAP3_I2C_STAT_NACK		__BIT(1)
#define  OMAP3_I2C_STAT_AL		__BIT(0)

#define OMAP3_I2C_WE		0x0c	/* Wakeup enable */
#define  OMAP3_I2C_WE_XDR_WE		__BIT(14)
#define  OMAP3_I2C_WE_RDR_WE		__BIT(13)
#define  OMAP3_I2C_WE_AAS_WE		__BIT(9)
#define  OMAP3_I2C_WE_BF_WE		__BIT(8)
#define  OMAP3_I2C_WE_STC_WE		__BIT(6)
#define  OMAP3_I2C_WE_GC_WE		__BIT(5)
#define  OMAP3_I2C_WE_DRDY_WE		__BIT(3)
#define  OMAP3_I2C_WE_ARDY_WE		__BIT(2)
#define  OMAP3_I2C_WE_NACK_WE		__BIT(1)
#define  OMAP3_I2C_WE_AL_WE		__BIT(0)

#define OMAP3_I2C_SYSS		0x10	/* Non-interrupt status */
#define  OMAP3_I2C_SYSS_RDONE		__BIT(0)

#define OMAP3_I2C_BUF		0x14	/* Receive DMA channel disabled */
#define  OMAP3_I2C_BUF_RDMA_EN		__BIT(15)
#define  OMAP3_I2C_BUF_RXFIFO_CLR	__BIT(14)
#define  OMAP3_I2C_BUF_RTRSH_MASK	__BITS(8,13)
#define  OMAP3_I2C_BUF_RTRSH_SHIFT	8
#define  OMAP3_I2C_BUF_XDMA_EN		__BIT(7)
#define  OMAP3_I2C_BUF_TXFIFO_CLR	__BIT(6)
#define  OMAP3_I2C_BUF_XTRSH_MASK	__BITS(0,5)
#define  OMAP3_I2C_BUF_XTRSH_SHIFT	0

#define OMAP3_I2C_CNT		0x18	/* Number of bytes in I2C payload */
#define  OMAP3_I2C_CNT_DCOUNT_MASK	__BITS(0,15)
#define  OMAP3_I2C_CNT_DCOUNT_SHIFT	0

#define OMAP3_I2C_DATA		0x1c	/* FIFO buffer */
#define  OMAP3_I2C_DATA_DATA_MASK	__BITS(0,7)
#define  OMAP3_I2C_DATA_DATA_SHIFT	0

#define OMAP3_I2C_SYSC		0x20	/* L4-Core interconnect control */
#define  OMAP3_I2C_CLOCKACTIVITY_MASK	__BITS(8,9)
#define  OMAP3_I2C_CLOCKACTIVITY_SHIFT	8
#define   OMAP3_I2C_CLOCKACTIVITY_DIS		0x0
#define   OMAP3_I2C_CLOCKACTIVITY_ICLK_EN	0x1
#define   OMAP3_I2C_CLOCKACTIVITY_FCLK_EN	0x2
#define   OMAP3_I2C_CLOCKACTIVITY_ICLK_FCLK_EN	0x3
#define  OMAP3_I2C_IDLEMODE_MASK	__BITS(3,4)
#define  OMAP3_I2C_IDLEMODE_SHIFT	3
#define   OMAP3_I2C_IDLEMODE_FORCEIDLE		0x0
#define   OMAP3_I2C_IDLEMODE_NOIDLE		0x1
#define   OMAP3_I2C_IDLEMODE_SMARTIDLE		0x2
#define  OMAP3_I2C_ENAWAKEUP		__BIT(2)
#define  OMAP3_I2C_SRST			__BIT(1)
#define  OMAP3_I2C_AUTOIDLE		__BIT(0)

#define OMAP3_I2C_CON		0x24	/* Function control */
#define  OMAP3_I2C_CON_I2C_EN		__BIT(15)
#define  OMAP3_I2C_CON_OPMODE_MASK	__BITS(12,13)
#define  OMAP3_I2C_CON_OPMODE_SHIFT		12
#define   OMAP3_I2C_CON_OPMODE_FASTSTD		0x0
#define   OMAP3_I2C_CON_OPMODE_HS		0x1
#define   OMAP3_I2C_CON_OPMODE_SCCB		0x2
#define  OMAP3_I2C_CON_STB		__BIT(11)
#define  OMAP3_I2C_CON_MST		__BIT(10)
#define  OMAP3_I2C_CON_TRX		__BIT(9)
#define  OMAP3_I2C_CON_XSA		__BIT(8)
#define  OMAP3_I2C_CON_XOA0		__BIT(7)
#define  OMAP3_I2C_CON_XOA1		__BIT(6)
#define  OMAP3_I2C_CON_XOA2		__BIT(5)
#define  OMAP3_I2C_CON_XOA3		__BIT(4)
#define  OMAP3_I2C_CON_STP		__BIT(1)
#define  OMAP3_I2C_CON_STT		__BIT(0)

#define OMAP3_I2C_OA0		0x28	/* Own address 0 */
#define  OMAP3_I2C_OA0_MCODE_MASK	__BITS(13,15)
#define  OMAP3_I2C_OA0_MCODE_SHIFT	13
#define  OMAP3_I2C_OA0_OA_MASK		__BITS(0,9)
#define  OMAP3_I2C_OA0_OA_SHIFT		0

#define OMAP3_I2C_SA		0x2c	/* Slave address */
#define  OMAP3_I2C_SA_MASK		__BITS(0,9)
#define  OMAP3_I2C_SA_SHIFT		0

#define OMAP3_I2C_PSC		0x30	/* Prescale sampling clock */
#define  OMAP3_I2C_PSC_MASK		__BITS(0,7)
#define  OMAP3_I2C_PSC_SHIFT		0

#define OMAP3_I2C_SCLL		0x34	/* SCL low time (master) */
#define  OMAP3_I2C_SCLL_HSSCLL_MASK	__BITS(8,15)
#define  OMAP3_I2C_SCLL_HSSCLL_SHIFT	8
#define  OMAP3_I2C_SCLL_SCLL_MASK	__BITS(0,7)
#define  OMAP3_I2C_SCLL_SCLL_SHIFT	0

#define OMAP3_I2C_SCLH		0x38	/* SCL high time (master) */
#define  OMAP3_I2C_SCLH_HSSCLH_MASK	__BITS(8,15)
#define  OMAP3_I2C_SCLH_HSSCLH_SHIFT	8
#define  OMAP3_I2C_SCLH_SCLH_MASK	__BITS(0,7)
#define  OMAP3_I2C_SCLH_SCLH_SHIFT	0

#define OMAP3_I2C_SYSTEST	0x3c	/* System tests */
#define  OMAP3_I2C_SYSTEST_ST_EN	__BIT(15)
#define  OMAP3_I2C_SYSTEST_FREE		__BIT(14)
#define  OMAP3_I2C_SYSTEST_TMODE_MASK	__BITS(12,13)
#define  OMAP3_I2C_SYSTEST_TMODE_SHIFT	12
#define  OMAP3_I2C_SYSTEST_SSB		__BIT(11)
#define  OMAP3_I2C_SYSTEST_SCCBE_O	__BIT(4)
#define  OMAP3_I2C_SYSTEST_SCL_I	__BIT(3)
#define  OMAP3_I2C_SYSTEST_SCL_O	__BIT(2)
#define  OMAP3_I2C_SYSTEST_SDA_I	__BIT(1)
#define  OMAP3_I2C_SYSTEST_SDA_O	__BIT(0)

#define OMAP3_I2C_BUFSTAT	0x40	/* FIFO status */
#define  OMAP3_I2C_BUFSTAT_FIFODEPTH_MASK __BITS(14,15)
#define  OMAP3_I2C_BUFSTAT_FIFODEPTH_SHIFT 14
#define  OMAP3_I2C_BUFSTAT_RXSTAT_MASK	__BITS(8,13)
#define  OMAP3_I2C_BUFSTAT_RXSTAT_SHIFT	8
#define  OMAP3_I2C_BUFSTAT_TXSTAT_MASK	__BITS(0,5)
#define  OMAP3_I2C_BUFSTAT_TXSTAT_SHIFT	0

#define OMAP3_I2C_OA1		0x44	/* Own address 1 */
#define  OMAP3_I2C_OA1_OA_MASK		__BITS(0,9)
#define  OMAP3_I2C_OA1_OA_SHIFT		0

#define OMAP3_I2C_OA2		0x48	/* Own address 2 */
#define  OMAP3_I2C_OA2_OA_MASK		__BITS(0,9)
#define  OMAP3_I2C_OA2_OA_SHIFT		0

#define OMAP3_I2C_OA3		0x4c	/* Own address 3 */
#define  OMAP3_I2C_OA3_OA_MASK		__BITS(0,9)
#define  OMAP3_I2C_OA3_OA_SHIFT		0

#define OMAP3_I2C_ACTOA		0x50	/* Active own addresses (slave) */
#define  OMAP3_I2C_ACTOA_OA3_ACT	__BIT(3)
#define  OMAP3_I2C_ACTOA_OA2_ACT	__BIT(2)
#define  OMAP3_I2C_ACTOA_OA1_ACT	__BIT(1)
#define  OMAP3_I2C_ACTOA_OA0_ACT	__BIT(0)

#define OMAP3_I2C_SBLOCK	0x54	/* Bus locking (slave) */
#define  OMAP3_I2C_SBLOCK_OA3_EN	__BIT(3)
#define  OMAP3_I2C_SBLOCK_OA2_EN	__BIT(2)
#define  OMAP3_I2C_SBLOCK_OA1_EN	__BIT(1)
#define  OMAP3_I2C_SBLOCK_OA0_EN	__BIT(0)

#endif /* !_OMAP3_I2CREG_H */
