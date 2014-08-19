/*	$NetBSD: imxcspireg.h,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
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

#ifndef	_ARM_IMX_IMXCSPIREG_H_
#define	_ARM_IMX_IMXCSPIREG_H_

#define	CSPI_RXDATA		0x00
#define	CSPI_TXDATA		0x04
#define	CSPI_CONREG		0x08
#ifdef IMX51
#define	 CSPI_CON_CS		__BITS(13, 12)
#define	 CSPI_CON_DRCTL		__BITS( 9,  8)
#define	 CSPI_CON_BITCOUNT	__BITS(31, 20)
#else
#define	 CSPI_CON_CS		__BITS(25, 24)
#define	 CSPI_CON_DRCTL		__BITS(21, 20)
#define	 CSPI_CON_BITCOUNT	__BITS(12,  8)
#endif
#define	 CSPI_CON_DIV		__BITS(18, 16)
#define	 CSPI_CON_SSPOL		__BIT(7)	/* SPI SS Polarity Select */
#define	 CSPI_CON_SSCTL		__BIT(6)	/* In master mode, this bit
						 * selects the output wave form
						 * for the SS signal.
						 */
#define	 CSPI_CON_PHA		__BIT(5)	/* PHA */
#define	 CSPI_CON_POL		__BIT(4)	/* POL */
#define	 CSPI_CON_SMC		__BIT(3)	/* SMC */
#define	 CSPI_CON_XCH		__BIT(2)	/* XCH */
#define	 CSPI_CON_MODE		__BIT(1)	/* MODE */
#define	 CSPI_CON_ENABLE	__BIT(0)	/* EN */
#define	CSPI_INTREG		0x0c
#define	 CSPI_INTR_ALL_EN	0x000001ff	/* All Intarruption Enabled */
#ifdef IMX51
#define	 CSPI_INTR_TC_EN	__BIT(7)	/* TX Complete */
#else
#define	 CSPI_INTR_TC_EN	__BIT(8)	/* TX Complete */
#define	 CSPI_INTR_BO_EN	__BIT(7)	/* Bit Counter Overflow */
#endif
#define	 CSPI_INTR_RO_EN	__BIT(6)	/* RXFIFO Overflow */
#define	 CSPI_INTR_RF_EN	__BIT(5)	/* RXFIFO Full */
#define	 CSPI_INTR_RH_EN	__BIT(4)	/* RXFIFO Half Full */
#define	 CSPI_INTR_RR_EN	__BIT(3)	/* RXFIFO Ready */
#define	 CSPI_INTR_TF_EN	__BIT(2)	/* TXFIFO Full */
#define	 CSPI_INTR_TH_EN	__BIT(1)	/* TXFIFO Half Empty */
#define	 CSPI_INTR_TE_EN	__BIT(0)	/* TXFIFO Empty */
#define	CSPI_DMAREG		0x10
#define	CSPI_STATREG		0x14
#ifdef IMX51
#define	 CSPI_STAT_CLR_TC	__BIT(7)	/* Clear TC of status register */
#define  CSPI_STAT_CLR		CSPI_STAT_CLR_TC
#else
#define	 CSPI_STAT_CLR_TC	__BIT(8)	/* Clear TC of status register */
#define	 CSPI_STAT_CLR_BO	__BIT(7)	/* Clear BO of status register */
#define  CSPI_STAT_CLR		(CSPI_STAT_CLR_TC | CSPI_STAT_CLR_BO)
#endif
#define	 CSPI_STAT_RO		__BIT(6)	/* RXFIFO Overflow */
#define	 CSPI_STAT_RF		__BIT(5)	/* RXFIFO Full */
#define	 CSPI_STAT_RH		__BIT(4)	/* RXFIFO Half Full */
#define	 CSPI_STAT_RR		__BIT(3)	/* RXFIFO Ready */
#define	 CSPI_STAT_TF		__BIT(2)	/* TXFIFO Full */
#define	 CSPI_STAT_TH		__BIT(1)	/* TXFIFO Half Empty */
#define	 CSPI_STAT_TE		__BIT(0)	/* TXFIFO Empty */
#define	CSPI_PERIODREG		0x18
#define	CSPI_TESTREG		0x1c

#define SPI_SIZE		0x100

#endif	/* _ARM_IMX_IMXCSPIREG_H_ */
