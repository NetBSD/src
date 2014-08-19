/*	$NetBSD: imxecspireg.h,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_IMX_IMXECSPIREG_H_
#define	_ARM_IMX_IMXECSPIREG_H_

#define	ECSPI_RXDATA		0x00
#define	ECSPI_TXDATA		0x04
#define	ECSPI_CONREG		0x08
#define	 ECSPI_CON_BITCOUNT	__BITS(31, 20)
#define	 ECSPI_CON_CS		__BITS(19,18)
#define	 ECSPI_CON_DRCTL	__BITS(17,16)
#define	 ECSPI_CON_PREDIV	__BITS(15,12)	/* PRE DIVIDER */
#define	 ECSPI_CON_DIV		__BITS(11, 8)	/* POST DIVIDER */
#define	 ECSPI_CON_MODE		__BITS( 7, 4)	/* MODE */
#define	 ECSPI_CON_SMC		__BIT(3)	/* SMC */
#define	 ECSPI_CON_XCH		__BIT(2)	/* XCH */
#define	 ECSPI_CON_HW		__BIT(1)	/* HW */
#define	 ECSPI_CON_ENABLE	__BIT(0)	/* EN */
#define	ECSPI_CONFIGREG		0x0c
#define	 ECSPI_CONFIG_HT_LEN	__BITS(28,24)	/* HT LENGHT */
#define	 ECSPI_CONFIG_SCLK_CTL	__BITS(23,20)	/* SCLK CTL */
#define	 ECSPI_CONFIG_DATA_CTL	__BITS(19,16)	/* DATA CTL */
#define	 ECSPI_CONFIG_SSB_POL	__BITS(15,12)	/* SSB POL */
#define	 ECSPI_CONFIG_SSB_CTL	__BITS(11, 8)	/* SSB CTL */
#define	 ECSPI_CONFIG_SCLK_POL	__BITS( 7, 4)	/* SCLK POL */
#define	 ECSPI_CONFIG_SCLK_PHA	__BITS( 3, 0)	/* SCLK PHA */
#define	ECSPI_INTREG		0x10
#define	 ECSPI_INTR_ALL_EN	__BITS( 7, 0)	/* All Intarruption Enabled */
#define	 ECSPI_INTR_TC_EN	__BIT(7)	/* TX Complete */
#define	 ECSPI_INTR_RO_EN	__BIT(6)	/* RXFIFO Overflow */
#define	 ECSPI_INTR_RF_EN	__BIT(5)	/* RXFIFO Full */
#define	 ECSPI_INTR_RD_EN	__BIT(4)	/* RXFIFO Data Request */
#define	 ECSPI_INTR_RR_EN	__BIT(3)	/* RXFIFO Ready */
#define	 ECSPI_INTR_TF_EN	__BIT(2)	/* TXFIFO Full */
#define	 ECSPI_INTR_TD_EN	__BIT(1)	/* TXFIFO Data Request */
#define	 ECSPI_INTR_TE_EN	__BIT(0)	/* TXFIFO Empty */
#define	ECSPI_DMAREG		0x14
#define	ECSPI_STATREG		0x18
#define	 ECSPI_STAT_CLR_TC	__BIT(7)	/* Clear Transfer Completed */
#define	 ECSPI_STAT_CLR_RO	__BIT(6)	/* Clear RXFIFO Overflow */
#define  ECSPI_STAT_CLR		ECSPI_STAT_CLR_TC
#define  ECSPI_STAT_RF		__BIT(5)
#define  ECSPI_STAT_RDR		__BIT(4)
#define  ECSPI_STAT_RR		__BIT(3)
#define  ECSPI_STAT_TF		__BIT(2)
#define  ECSPI_STAT_TDR		__BIT(1)
#define  ECSPI_STAT_TE		__BIT(0)
#define	ECSPI_PERIODREG		0x1c
#define	ECSPI_TESTREG		0x20
#define	ECSPI_MSGDATA		0x40

#define ECSPI_SIZE		0x50

#endif	/* _ARM_IMX_IMXECSPIREG_H_ */
