/*	$NetBSD: imxusbreg.h,v 1.1.20.2 2017/12/03 11:35:53 jdolecek Exp $	*/
/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
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

#ifndef _ARM_IMX_IMXUSBREG_H
#define _ARM_IMX_IMXUSBREG_H

#define	IMXUSB_ID		0x0000
#define	 IMXUSB_ID_ID		__BITS(5,0)
#define	 IMXUSB_ID_REVISION	__BITS(23,16)
#define	IMXUSB_HWGENERAL	0x0004
#define	IMXUSB_HWHOST		0x0008
#define	 HWHOST_HC		__BIT(0)
#define	 HWHOST_NPORT		__BITS(3,1)
#define	IMXUSB_HWDEVICE		0x000c
#define	 HWDEVICE_DC		__BIT(0)
#define	 HWDEVICE_DEVEP		__BITS(5,1)
#define	IMXUSB_HWTXBUF		0x0010
#define	IMXUSB_HWRXBUF		0x0014

#define	IMXUSB_EHCIREGS	0x0100

#define	IMXUSB_ULPIVIEW	0x0170
#define	 ULPI_WU	__BIT(31)
#define	 ULPI_RUN	__BIT(30)
#define	 ULPI_RW	__BIT(29)
#define	 ULPI_SS	__BIT(27)
#define	 ULPI_PORT	__BITS(26,24)
#define	 ULPI_ADDR	__BITS(23,16)
#define	 ULPI_DATRD	__BITS(15,8)
#define	 ULPI_DATWR	__BITS(7,0)

#define	IMXUSB_OTGSC	0x01A4
#define	 OTGSC_DPIE	__BIT(30)
#define	 OTGSC_1MSE	__BIT(29)
#define	 OTGSC_BSEIE	__BIT(28)
#define	 OTGSC_BSVIE	__BIT(27)
#define	 OTGSC_ASVIE	__BIT(26)
#define	 OTGSC_AVVIE	__BIT(25)
#define	 OTGSC_IDIE	__BIT(24)
#define	 OTGSC_DPIS	__BIT(22)
#define	 OTGSC_1MSS	__BIT(21)
#define	 OTGSC_BSEIS	__BIT(20)
#define	 OTGSC_BSVIS	__BIT(19)
#define	 OTGSC_ASVIS	__BIT(18)
#define	 OTGSC_AVVIS	__BIT(17)
#define	 OTGSC_IDIS	__BIT(16)
#define	 OTGSC_DPS	__BIT(14)
#define	 OTGSC_1MST	__BIT(13)
#define	 OTGSC_BSE	__BIT(12)
#define	 OTGSC_BSV	__BIT(11)
#define	 OTGSC_ASV	__BIT(10)
#define	 OTGSC_AVV	__BIT( 9)
#define	 OTGSC_ID	__BIT( 8)
#define	 OTGSC_IDPU	__BIT( 5)
#define	 OTGSC_DP	__BIT( 4)
#define	 OTGSC_OT	__BIT( 3)
#define	 OTGSC_VC	__BIT( 1)
#define	 OTGSC_VD	__BIT( 0)
#define	IMXUSB_USBMODE	0x01A8
#define	 USBMODE_VBPS	__BIT(5)	/* Vbus power selectt */
#define	 USBMODE_SDIS	__BIT(4)	/* Stream disable mode 1=act */
#define	 USBMODE_SLOM	__BIT(3)	/* setup lockouts on */
#define	 USBMODE_ES	__BIT(2)	/* Endian Select ES=1 */
#define	 USBMODE_CM	__BITS(1,0)	/* Controller mode */
#define	 USBMODE_CM_IDLE	__SHIFTIN(0,USBMODE_CM)
#define	 USBMODE_CM_DEVICE	__SHIFTIN(2,USBMODE_CM)
#define	 USBMODE_CM_HOST	__SHIFTIN(3,USBMODE_CM)

#define	IMXUSB_EHCI_SIZE	0x200


/* extension to PORTSCx register of EHCI. */
#define	PORTSC_PTS		__BITS(31,30)
#define	PORTSC_PTS_UTMI		__SHIFTIN(0,PORTSC_PTS)
#define	PORTSC_PTS_PHILIPS	__SHIFTIN(1,PORTSC_PTS) /* not in i.MX51*/
#define	PORTSC_PTS_ULPI		__SHIFTIN(2,PORTSC_PTS)
#define	PORTSC_PTS_SERIAL	__SHIFTIN(3,PORTSC_PTS)
#define	PORTSC_PTS2		__BIT(25)	/* iMX6,7 */

#define	PORTSC_STS	__BIT(29)	/* serial transeiver select */
#define	PORTSC_PTW	__BIT(28)	/* parallel transceiver width */
#define	PORTSC_PTW_8	0
#define	PORTSC_PTW_16	PORTSC_PTW
#define	PORTSC_PSPD	__BITS(26,27)	/* port speed (RO) */
#define	PORTSC_PFSC	__BIT(24)	/* port force full speed */
#define	PORTSC_PHCD	__BIT(23)	/* PHY low power suspend */

#endif	/* _ARM_IMX_IMXUSBREG_H */
