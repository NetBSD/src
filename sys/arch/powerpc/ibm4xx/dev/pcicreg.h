/*	$NetBSD: pcicreg.h,v 1.2 2023/06/24 05:31:05 msaitoh Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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

#ifndef _IBM4XX_PCICREG_H_
#define	_IBM4XX_PCICREG_H_

/* PCI Configuration Registers */
#define	PCIC_CFGADDR		0x00
#define	PCIC_CFGDATA		0x04

#define	PCIC_VENDID		0x00
#define	PCIC_DEVID		0x02
#define	PCIC_CMD		0x04
#define	PCIC_STATUS		0x06
#define	PCIC_REVID		0x08
#define	PCIC_CLS		0x09
#define	PCIC_CACHELS		0x0c
#define	PCIC_LATTIM		0x0d
#define	PCIC_HDTYPE		0x0e
#define	PCIC_BIST		0x0f
#define	PCIC_BAR0		0x10
#define	PCIC_BAR1		0x14		/* PCI name */
#define	PCIC_PTM1BAR		PCIC_BAR1	/* 405GP name */
#define	PCIC_BAR2		0x18		/* PCI name */
#define	PCIC_PTM2BAR		PCIC_BAR2	/* 405GP name */
#define	PCIC_BAR3		0x1C
#define	PCIC_BAR4		0x20
#define	PCIC_BAR5		0x24
#define	PCIC_SBSYSVID		0x2c
#define	PCIC_SBSYSID		0x2e
#define	PCIC_CAP		0x34
#define	PCIC_INTLN		0x3c
#define	PCIC_INTPN		0x3d
#define	PCIC_MINGNT		0x3e
#define	PCIC_MAXLTNCY		0x3f

#define	PCIC_ICS		0x44	/* 405GP specific parameters */
#define	PCIC_ERREN		0x48
#define	PCIC_ERRSTS		0x49
#define	PCIC_BRDGOPT1		0x4a
#define	PCIC_PLBBESR0		0x4c
#define	PCIC_PLBBESR1		0x50
#define	PCIC_PLBBEAR		0x54
#define	PCIC_CAPID		0x58
#define	PCIC_NEXTIPTR		0x59
#define	PCIC_PMC		0x5a
#define	PCIC_PMCSR		0x5c
#define	PCIC_PMCSRBSE		0x5e
#define	PCIC_DATA		0x5f
#define	PCIC_BRDGOPT2		0x60
#define	PCIC_PMSCRR		0x64


/* PCI Bridge Local Configuration Registers (0xef400000 0xef40003f - 64 bytes) */
#define	PCIL_PMM0LA		0x00	/* PCI Master Map 0: Local Address */
#define	PCIL_PMM0MA		0x04	/*		     Mask/Attribute */
#define	PCIL_PMM0PCILA		0x08	/*		     PCI Low Address */
#define	PCIL_PMM0PCIHA		0x0c	/*		     PCI High Address */
#define	PCIL_PMM1LA		0x10
#define	PCIL_PMM1MA		0x14
#define	PCIL_PMM1PCILA		0x18
#define	PCIL_PMM1PCIHA		0x1c
#define	PCIL_PMM2LA		0x20
#define	PCIL_PMM2MA		0x24
#define	PCIL_PMM2PCILA		0x28
#define	PCIL_PMM2PCIHA		0x2c
#define	PCIL_PTM1MS		0x30
#define	PCIL_PTM1LA		0x34
#define	PCIL_PTM2MS		0x38
#define	PCIL_PTM2LA		0x3c
#endif	/* _IBM4XX_PCICREG_H_ */
