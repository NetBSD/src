/*	$NetBSD: imx7_usbreg.h,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2016 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX7_USBREG_H_
#define _ARM_IMX_IMX7_USBREG_H_

#include <sys/cdefs.h>

/*
 * USBC:
 *   OTG1: 0x30b10000
 *        +0x00000000: USBCore
 *        +0x00000200: USB NonCore
 *   OTG2: 0x30b20000
 *        +0x00000000: USBCore
 *        +0x00000200: USB NonCore
 *   HSIC: 0x30b30000
 *        +0x00000000: USBCore
 *        +0x00000200: USB NonCore
 */
#define IMX7_USBC_SIZE				0x00030000
#define IMX7_USBC_EHCISIZE			0x00010000

/* USB Core */
#define USB_X_ID				0x00000000
#define USB_X_PORTSC1				0x00000184
#define  USB_X_PORTSC1_PE			__BIT(2)
#define USB_X_OTGSC				0x000001a4
#define  USB_X_OTGSC_PTS_1			__BITS(31,30)
#define  USB_X_OTGSC_STS			__BIT(29)
#define  USB_X_OTGSC_PTW			__BIT(28)
#define  USB_X_OTGSC_PSPD			__BITS(27,26)
#define  USB_X_OTGSC_PTS_2			__BIT(25)
#define  USB_X_OTGSC_PFSC			__BIT(24)
#define  USB_X_OTGSC_PHCD			__BIT(23)
#define  USB_X_OTGSC_WKOC			__BIT(22)
#define  USB_X_OTGSC_WKDC			__BIT(21)
#define  USB_X_OTGSC_WKCN			__BIT(20)
#define  USB_X_OTGSC_PTC			__BITS(19,16)
#define  USB_X_OTGSC_PIC			__BITS(15,14)
#define  USB_X_OTGSC_PO				__BIT(13)
#define  USB_X_OTGSC_PP				__BIT(12)
#define  USB_X_OTGSC_LS				__BITS(11,10)
#define  USB_X_OTGSC_HSP			__BIT(9)
#define  USB_X_OTGSC_PR				__BIT(8)
#define  USB_X_OTGSC_SUSP			__BIT(7)
#define  USB_X_OTGSC_FPR			__BIT(6)
#define  USB_X_OTGSC_OCC			__BIT(5)
#define  USB_X_OTGSC_OCA			__BIT(4)
#define  USB_X_OTGSC_PEC			__BIT(3)
#define  USB_X_OTGSC_PE				__BIT(2)
#define  USB_X_OTGSC_CSC			__BIT(1)
#define  USB_X_OTGSC_CCS			__BIT(0)
#define USB_X_USBMODE				0x000001a8
#define  USB_X_USBMODE_SDIS			__BIT(4)
#define  USB_X_USBMODE_SLOM			__BIT(3)
#define  USB_X_USBMODE_ES			__BIT(2)
#define  USB_X_USBMODE_CM			__BITS(1,0)

/* USB Non-Core */
#define USBNC_N_CTRL1				0x00000200
#define  USBNC_N_CTRL1_WIR			__BIT(31)
#define  USBNC_N_CTRL1_WKUP_DPDM_EN		__BIT(29)
#define  USBNC_N_CTRL1_WKUP_VBUS_EN		__BIT(17)
#define  USBNC_N_CTRL1_WKUP_ID_EN		__BIT(16)
#define  USBNC_N_CTRL1_WKUP_SW			__BIT(15)
#define  USBNC_N_CTRL1_WKUP_SW_EN		__BIT(14)
#define  USBNC_N_CTRL1_WIE			__BIT(10)
#define  USBNC_N_CTRL1_PWR_POL			__BIT(9)
#define  USBNC_N_CTRL1_OVER_CUR_POL		__BIT(8)
#define  USBNC_N_CTRL1_OVER_CUR_DIS		__BIT(7)
#define USBNC_N_CTRL2				0x00000204
#define USBNC_N_PHY_CFG1			0x00000230
#define USBNC_N_PHY_CFG2			0x00000234
#define USBNC_N_PHY_STATUS			0x0000023c

#define USBNC_ADP_CFG1				0x00000250
#define USBNC_ADP_CFG2				0x00000254
#define USBNC_ADP_STATUS			0x00000258

#endif /* _ARM_IMX_IMX7_USBREG_H_ */
