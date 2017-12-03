/*	$NetBSD: imx7_srcreg.h,v 1.2.14.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX7_SRCREG_H_
#define _ARM_IMX_IMX7_SRCREG_H_

#include <sys/cdefs.h>

/* SRC - System Reset Controller */
#define SRC_SCR					0x00000000
#define SRC_A7RCR0				0x00000004
#define  SRC_A7RCR0_A7_L2_RESET			__BIT(21)
#define  SRC_A7RCR0_A7_CORE_RESET0		__BIT(5)
#define  SRC_A7RCR0_A7_CORE_RESET1		__BIT(4)
#define  SRC_A7RCR0_A7_CORE_POR_RESET1		__BIT(1)
#define  SRC_A7RCR0_A7_CORE_POR_RESET0		__BIT(0)
#define SRC_A7RCR1				0x00000008
#define  SRC_A7RCR1_A7_CORE1_ENABLE		__BIT(1)
#define SRC_M4RCR				0x0000000c
#define  SRC_M4RCR_ENABLE_M4			__BIT(3)
#define  SRC_M4RCR_SW_M4P_RST			__BIT(2)
#define  SRC_M4RCR_SW_M4C_RST			__BIT(1)
#define  SRC_M4RCR_SW_M4C_NON_SCLR_RST		__BIT(0)
#define SRC_ERCR				0x00000014
#define SRC_HSICPHY_RCR				0x0000001c
#define  SRC_HSICPHY_PORT_RST			__BIT(1)
#define SRC_USBOPHY1_RCR			0x00000020
#define  SRC_USBPHY1_PORT_RST			__BIT(1)
#define  SRC_USBPHY1_PORT_POR			__BIT(0)
#define SRC_USBOPHY2_RCR			0x00000024
#define  SRC_USBPHY2_PORT_RST			__BIT(1)
#define  SRC_USBPHY2_PORT_POR			__BIT(0)
#define SRC_MIPIPHY_RCR				0x00000028
#define SRC_PCIEPHY_RCR				0x0000002c
#define SRC_SBMR1				0x00000058
#define SRC_SRSR				0x0000005c
#define SRC_SISR				0x00000068
#define SRC_SIMR				0x0000006c
#define SRC_SBMR2				0x00000070
#define SRC_GPR1				0x00000074
#define SRC_GPR2				0x00000078
#define SRC_GPR3				0x0000007c
#define SRC_GPR4				0x00000080
#define SRC_GPR5				0x00000084
#define SRC_GPR6				0x00000088
#define SRC_GPR7				0x0000008c
#define SRC_GPR8				0x00000090
#define SRC_GPR9				0x00000094
#define SRC_GPR10				0x00000098
#define SRC_DDRC_RCR				0x00001000

#endif /* _ARM_IMX_IMX7_SRCREG_H_ */
