/* $NetBSD: aupcireg.h,v 1.2.10.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _MIPS_ALCHEMY_DEV_AUPCIREG_H
#define	_MIPS_ALCHEMY_DEV_AUPCIREG_H

#define	AUPCI_CMEM			0x0000
#define	  AUPCI_CMEM_HC			(1UL<<31)	/* host config */
#define   AUPCI_CMEM_E			(1UL<<28)	/* cmem enable */

#define AUPCI_CONFIG			0x0004
#define	  AUPCI_CONFIG_EADDRH_SHIFT	28		/* bits 32-35 */
#define	  AUPCI_CONFIG_ERD		(1UL<<27)	/* error direction */
#define	  AUPCI_CONFIG_ET		(1UL<<26)	/* error target */
#define	  AUPCI_CONFIG_EF		(1UL<<25)	/* fatal error */
#define	  AUPCI_CONFIG_EP		(1UL<<24)	/* parity error */
#define	  AUPCI_CONFIG_EM		(1UL<<23)	/* multiple errors */
#define	  AUPCI_CONfIG_BM		(1UL<<22)	/* bad master */
#define	  AUPCI_CONFIG_PD		(1UL<<20)	/* PCI disable */
#define	  AUPCI_CONFIG_BME		(1UL<<19)	/* byte mask enable */
#define   AUPCI_CONFIG_DR		(1UL<<18)	/* drive mode */
#define	  AUPCI_CONFIG_NC		(1UL<<16)	/* non-coherent */
#define	  AUPCI_CONFIG_IE		(1UL<<15)	/* interrupt enable */
#define	  AUPCI_CONFIG_IP		(1UL<<13)	/* perr int enable */
#define	  AUPCI_CONFIG_IS		(1UL<<12)	/* serr int enable */
#define	  AUPCI_CONFIG_IMM		(1UL<<11)	/* master abort int */
#define	  AUPCI_CONFIG_ITM		(1UL<<10)	/* target abort int */
#define	  AUPCI_CONFIG_ITT		(1UL<<9)	/* target abort int */
#define	  AUPCI_CONFIG_IPB		(1UL<<8)	/* perr rec int */
#define	  AUPCI_CONFIG_SIC_SHIFT	6
#define	  AUPCI_CONFIG_SIC_NONE		0
#define	  AUPCI_CONFIG_SIC_ADDR		(1UL<<6)
#define	  AUPCI_CONFIG_SIC_DATA		(2UL<<6)
#define	  AUPCI_CONFIG_SIC_ALL		(3UL<<6)
#define	  AUPCI_CONFIG_SIC_MASK		(3UL<<6)
#define   AUPCI_CONFIG_ST		(1UL<<5)	/* swap on target */
#define	  AUPCI_CONFIG_SM		(1UL<<4)	/* swap on master */
#define	  AUPCI_CONFIG_AEN		(1UL<<3)	/* enable arbiter */
#define	  AUPCI_CONFIG_R2H		(1UL<<2)	/* req 2 high pri */
#define	  AUPCI_CONFIG_R1H		(1UL<<1)	/* req 1 high pri */
#define	  AUPCI_CONFIG_CH		(1UL<<0)	/* cpu high pri */

#define	AUPCI_B2BMASK			0x0008
#define	  AUPCI_B2BMASK_SHIFT		16
#define AUPCI_B2BBASE0			0x000C
#define   AUPCI_B2BASE0_SHIFT		16
#define	AUPCI_B2BBASE1			0x0010
#define   AUPCI_B2BASE1_SHIFT		16
#define	AUPCI_MWMASK			0x0014
#define	  AUPCI_MWMASK_SHIFT		16
#define	AUPCI_MWBASE			0x0018
#define	  AUPCI_MWBASE_SHIFT		16
#define	AUPCI_ERRADDR			0x001C
#define	AUPCI_SPECINTACK		0x0020
#define	AUPCI_PRCFG			0x0024
#define   AUPCI_PRCFG_BLM_SHIFT		3
#define	  AUPCI_PRCFG_AM		(1UL<<9)	/* abort mask */
#define	  AUPCI_PRCFG_DM		(1UL<<8)	/* done mask */
#define	  AUPCI_PRCFG_BS_SHIFT		4
#define	  AUPCI_PRCFG_ADDR_HIGH_SHIFT	0
#define	AUPCI_PRADDR			0x0028
#define	AUPCI_PRSTAT			0x002C
#define   AUPCI_PRSTAT_AI		(1UL<<9)	/* posted read abort */
#define	  AUPCI_PRSTAT_DI		(1UL<<8)	/* posted read done */
#define	  AUPCI_PRSTAT_PEND		(1UL<<0)	/* posted read pend */

#define	AUPCI_ID		0x0100
#define	AUPCI_COMMAND_STATUS	0x0104
#define	AUPCI_CLASS		0x0108
#define	AUPCI_MBAR		0x0110

#endif	/* _MIPS_ALCHEMY_DEV_AUPCIREG_H */
