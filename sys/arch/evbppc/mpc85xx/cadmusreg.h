/*	$NetBSD: cadmusreg.h,v 1.1.4.1 2011/06/06 09:05:32 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _MPC85XX_CADMUSREG_H_
#define _MPC85XX_CADMUSREG_H_

#define	CM_BASE		0xF8004000

#define	CM_VER		0x00	/* System version register */
#define	CM_VER_ID	__BITS(4,7)
#define	CM_VER_ID_GET(n) __SHIFTOUT((n), CM_VER_ID)
#define	CM_VER_REV	__BITS(0,3)
#define	CM_VER_REV_GET(n) __SHIFTOUT((n), CM_VER_ID)
#define	CM_CSR		0x01	/* General control/status register */
#define	CM_CSR_USER	__BITS(6,7)
#define	CM_CSR_USER_GET(n) __SHIFTOUT((n), CM_CSR_USER)
#define	CM_CSR_EPHY	__BITS(1,3)
#define	CM_CSR_EPHY_GET(n) __SHIFTOUT((n), CM_CSR_EPHY)
#define	CM_CSR_LED	__BIT(0)
#define	CM_RST		0x02	/* Reset control register */
#define	CM_RST_XRSTEN	__BIT(7) /* Enable the NVRAM watchdog timr to function as a general reset input */
#define	CM_RST_PHYRST	__BIT(6) /* Reset the Ethernet PHY */
#define	CM_RST_ATM1RST	__BIT(5) /* Reset the FCC1/ATM1 PHYS */
#define	CM_RST_ATM2RST	__BIT(4) /* Reset the FCC2/ATM2 PHY */
#define	CM_RST_MEMRST	__BIT(3) /* Reset the memory devices on the daughter card */
#define	CM_RST_UTRST	__BIT(2) /* Reset the TCOM/ECOM boards */
#define	CM_RST_HRESET	__BIT(1) /* Assert HRESET */
#define	CM_RST_SRESET	__BIT(0) /* Assert SRESET */
#define	CM_LED		0x05	/* LED data register */
#define	CM_PCI		0x06	/* PCI control/status register */
#define	CM_PCI_M66O	__BIT(7)
#define	CM_PCI_PCIXCO	__BIT(6)
#define	CM_PCI_M66S	__BIT(5) /* PCI V2.3 mode or earlier running at 66MHz */
#define	CM_PCI_DUAL	__BIT(4) /* Daughter card has selected dual PCI-mode */
#define	CM_PCI_PSPEED	__BITS(2,3) /* detected PCI speed */
#define	CM_PCI_PSPEED_33 __SHIFTIN(0, CM_PCI_PSPEED)
#define	CM_PCI_PSPEED_66 __SHIFTIN(1, CM_PCI_PSPEED)
#define	CM_PCI_PCIX	__BIT(1) /* the PCI edge connector is connected to a PCI-X backplace */
#define	CM_PCI_PCIEN	__BIT(0) /* if 1, the PCI backplane is not present */

#define	CM_DMA		0x07	/* DMA control register */
#define	CM_DMA_DMARQ0	__BIT(6)
#define	CM_DMA_DMACK0	__BIT(5)
#define	CM_DMA_DMADN0	__BIT(4)
#define	CM_DMA_DMARQ1	__BIT(2)
#define	CM_DMA_DMACK1	__BIT(1)
#define	CM_DMA_DMADN1	__BIT(0)

#endif /* !_MPC85XX_CADMUSREG_H_ */
