/*	$NetBSD: cprcreg.h,v 1.1 2002/07/05 13:31:51 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_CPRCREG_H
#define _SH5_CPRCREG_H

/*
 * Register Offsets for the Clock, Power, Watchdog and Reset Controller Module
 */
#define	CPRC_REG_FRQ		0x00	/* Clock: Frequency Control Register */
#define	CPRC_REG_PLL		0x08	/* Clock: PLL1 Control Register */
#define	CPRC_REG_WTCNT		0x10	/* Watchdog: Count Register */
#define	CPRC_REG_WTCS		0x18	/* Watchdog: Control/Status Register */
#define	CPRC_REG_MSTP		0x20	/* Power: Module Stop Register */
#define	CPRC_REG_MSTPACK	0x28	/* Power: Module Stop Ack Register */
#define	CPRC_REG_STBCR		0x30	/* Power: Control Register */
#define	CPRC_REG_RST		0x38	/* Reset: Control Register */

#define	CPRC_REG_SIZE		0x40

/*
 * Bit definitions for CPRC_REG_FRQ
 */
#define	CPRC_FRQ_MASK		0x07	/* Mask for clock divider fields */
#define	CPRC_FRQ_EMC_SHIFT	0	/* External Memory Clk Divider Ratio */
#define	CPRC_FRQ_BMC_SHIFT	3	/* SuperHyway Clock Divider Ratio */
#define	CPRC_FRQ_IFC_SHIFT	6	/* CPU Clock Divider Ratio */
#define	CPRC_FRQ_PBC_SHIFT	12	/* Peripheral Clock Divider Ratio */
#define	CPRC_FRQ_PCC_SHIFT	15	/* PCIbus Clock Divider Ratio */
#define	CPRC_FRQ_FMC_SHIFT	18	/* Flash Memory I/F Clk Divider Ratio */
#define	CPRC_FRQ_SBC_SHIFT	21	/* ST Legacy Bus Clock Divider Ratio */
#define	CPRC_FRQ_PLL2EN		0x0200	/* PLL2 Enable (PLL2 not in eval) */
#define	CPRC_FRQ_PLL1EN		0x0400	/* PLL1 Enable */
#define	CPRC_FRQ_CKOEN		0x0800	/* Clock output enable (Not in eval) */

/*
 * Given an encoded divider ratio from the CPRC_REG_FRQ register, this
 * macro evaluates to "1/actual ratio" it represents.
 *
 * So, the encoding "0" represents the ratio 1/2. In this case,
 * the macro evaluates to "2".
 */
#define	CPRC_FRQ2DIV(f)	(((f)<6)?(((f)+1)*2):(((f)==6)?16:24))

/*
 * Bit definitions for CPRC_REG_PLL
 */
#define	CPRC_PLL_MDIV_MASK	0xff	/* PLL1 Pre-divider */
#define	CPRC_PLL_MDIV_SHIFT	0
#define	CPRC_PLL_NDIV_MASK	0xff	/* PLL1 Feedback divider */
#define	CPRC_PLL_NDIV_SHIFT	8
#define	CPRC_PLL_PDIV_MASK	0x07	/* PLL1 Post divider */
#define	CPRC_PLL_PDIV_SHIFT	16
#define	CPRC_PLL_SETUP_MASK	0x1ff	/* PLL1 Loop characteristics */
#define	CPRC_PLL_SETUP_SHIFT	19
#define	CPRC_PLL_ENABLE_MASK	0x03	/* PLL1 Enabling Truth Table */
#define	CPRC_PLL_ENABLE_SHIFT	28
#define	CPRC_PLL_LOCKED		(1<<30)	/* PLL1 Locked */
#define	CPRC_PLL_POWER		(1<<31)	/* PLL1 Power Control */

#endif /* _SH5_CPRCREG_H */
