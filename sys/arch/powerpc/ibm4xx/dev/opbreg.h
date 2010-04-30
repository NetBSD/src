/*	$NetBSD: opbreg.h,v 1.1.138.1 2010/04/30 14:39:42 uebayasi Exp $	*/

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

#ifndef _IBM4XX_OPBREG_H_
#define	_IBM4XX_OPBREG_H_

#define OPBREG_SIZE	0x1000

/* OPB Arbiter Registers */
#define	OPBA_PR			0x00	/* Priority Register */
#define	OPBA_CR			0x01	/* Control Register */


/* ZMII Bridge (440EP/440GP/440GX) */
#define ZMII0_SIZE		0x10
#define ZMII0_FER		0x0	/* Function Enable Register */
#define   FER_MDI_MASK		  0x88880000	/* MDI enable */
#define   FER_MDI(emac)		  (1 << (31 - ((emac) << 2)))
#define   FER__MII_MASK		  0x7
#define   FER__MII_MII		  0x1		/* MII Enable */
#define   FER__MII_RMII		  0x2		/* ZMII (or RMII) Enable */
#define   FER__MII_SMII		  0x4		/* SMII Enable */
#define   FER__MII(emac, mii)	  ((mii) << (28 - ((emac) << 2)))
#define ZMII0_SSR		0x4	/* Speed Selection Register */
#define   SSR_SCI(emac)		  (0x4 << (28 - ((emac) << 2))) /* Suppress Collision Indication */
#define   SSR_FSS(emac)		  (0x2 << (28 - ((emac) << 2))) /* Force Speed Selection */
#define   SSR_SP_10MBPS		  0x0
#define   SSR_SP_100MBPS	  0x2
#define   SSR_ZSP(emac, sp)	  ((sp) << (27 - ((emac) << 2))) /* Speed Selection */
#define ZMII0_SMIISR		0x8	/* SMII Status Register */
#define   SMIISR_SHIFT(emac)	  (24 - ((emac) << 3))
#define   SMIISR_MASK		  0xff
#define   SMIISR_E1		  0x01		/* RxD Set to 1 */
#define   SMIISR_EC		  0x02		/* RxD False Carrier Detected */
#define   SMIISR_EN_INVALID	  0x00		/* RxD Nibble  Invalid */
#define   SMIISR_EN_VALID	  0x04		/* RxD Nibble  Valid */
#define   SMIISR_EJ_OK		  0x00		/* RxD Jabber  OK */
#define   SMIISR_EJ_ERROR	  0x08		/* RxD Jabber  Error */
#define   SMIISR_EL_DOWN	  0x00		/* RxD Link  Down */
#define   SMIISR_EL_UP		  0x10		/* RxD Link  Up */
#define   SMIISR_ED_HALF	  0x00		/* RxD Duplex  Half */
#define   SMIISR_ED_FULL	  0x20		/* RxD Duplex  Full */
#define   SMIISR_ES_10		  0x00		/* RxD Speed  10MBit */
#define   SMIISR_ES_100		  0x40		/* RxD Speed  100MBit */
#define   SMIISR_EF		  0x80		/* RxD from Previous Frame */

#endif	/* _IBM4XX_OPBREG_H_ */
