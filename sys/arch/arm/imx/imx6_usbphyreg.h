/*	$NetBSD: imx6_usbphyreg.h,v 1.1.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2017  Genetec Corporation.  All rights reserved.
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

#ifndef _ARM_IMX_IMX6_USBPHYREG_H
#define _ARM_IMX_IMX6_USBPHYREG_H

#include <sys/cdefs.h>

#define USBPHY_PWD			0x00000000
#define USBPHY_PWD_SET			0x00000004
#define USBPHY_PWD_CLR			0x00000008
#define USBPHY_PWD_TOG			0x0000000c
#define USBPHY_TX			0x00000010
#define USBPHY_TX_SET			0x00000014
#define USBPHY_TX_CLR			0x00000018
#define USBPHY_TX_TOG			0x0000001c
#define  USBPHY_TX_USBPHY_TX_EDGECTRL	__BITS(28, 26)
#define  USBPHY_TX_TXCAL45DP		__BITS(19, 16)
#define  USBPHY_TX_TXCAL45DN		__BITS(11, 8)
#define  USBPHY_TX_D_CAL		__BITS(3, 0)
#define USBPHY_RX			0x00000020
#define USBPHY_RX_SET			0x00000024
#define USBPHY_RX_CLR			0x00000028
#define USBPHY_RX_TOG			0x0000002c
#define USBPHY_CTRL			0x00000030
#define USBPHY_CTRL_SET			0x00000034
#define USBPHY_CTRL_CLR			0x00000038
#define USBPHY_CTRL_TOG			0x0000003c
#define  USBPHY_CTRL_SFTRST		__BIT(31)
#define  USBPHY_CTRL_CLKGATE		__BIT(30)
#define  USBPHY_CTRL_ENUTMILEVEL3	__BIT(15)
#define  USBPHY_CTRL_ENUTMILEVEL2	__BIT(14)
#define USBPHY_STATUS			0x00000040
#define USBPHY_DEBUG			0x00000050
#define USBPHY_DEBUG0_STATUS		0x00000060
#define USBPHY_DEBUG1			0x00000070
#define USBPHY_VERSION			0x00000080

#endif /* _ARM_IMX_IMX6_USBPHYREG_H */
