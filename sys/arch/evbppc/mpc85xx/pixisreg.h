/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _MPC85XX_PIXISREG_H_
#define _MPC85XX_PIXISREG_H_

#define	PX_BASE		0xffdf0000

#define	PX_ID		0x00	/* System ID register */
#define	PX_VER		0x01	/* System version register */
#define	PX_VER_ID	__BITS(4,7)
#define	PX_VER_ID_GET(n) __SHIFTOUT((n), PX_VER_ID)
#define	PX_VER_REV	__BITS(0,3)
#define	PX_VER_REV_GET(n) __SHIFTOUT((n), PX_VER_ID)
#define	PX_PVER		0x02	/* System version register */
#define	PX_CSR		0x03	/* General control/status register */
#define	PX_CSR_ASLEEP	__BIT(6)
#define	PX_CSR_EVES	__BITS(2,3)
#define	PX_CSR_EVES_GET(n)	__SHIFTOUT((n), PX_CSR_EVES)
#define	PX_CSR_PASS	__BIT(1)
#define	PX_CSR_FAIL	__BIT(0)
#define	PX_RST		0x04	/* Reset control register */
#define	PX_RST_ALL	__BIT(7) /* if set to 0 a full system reset is initiated */
#define	PX_RST_PCIE1	__BIT(6) /* If 1: RST_PCIE1* is deasserted and the slot is out of reset. */
#define	PX_RST_PCIE2	__BIT(5) /* If 1: RST_PCIE2* is deasserted and the slot is out of reset. */
#define	PX_RST_PCIE3	__BIT(4) /* If 1: RST_PCIE3* is deasserted and the slot is out of reset. */
#define	PX_RST_USB	__BIT(3) /* If 0: RST_USB3300 is deasserted and the devices are out of reset. */
#define	PX_RST_PHY	__BIT(2) /* If 1: PHY_RST* is deasserted and the device is out of reset. */
#define	PX_RST_LB	__BIT(1) /* If 1: LB_RST* is deasserted and the device is out of reset. */
#define	PX_RST_GEN	__BIT(0) /* If 1: GEN_RST* is deasserted and the devices are out of reset. */
#define	PX_RST2		0x05	/* Reset control register 2 */
#define	PX_RST2_SGMII	__BIT(1) /* If 1: RST_SGMII_SLOT* is deasserted and the device is out of reset. */
#define	PX_RST2_PCI	__BIT(0) /* If 1: RST_PCI_SLOT* is deasserted and the devices are out of reset. */
#define	PX_AUX1		0x06	/* Auxiliary 1 register */
#define	PX_SPD		0x07	/* Speed register */
#define	PX_SPD_SYSCLK	__BITS(0,2)
#define	PX_SPD_SYSCLK_GET(n)	__SHIFTOUT((n), PX_SPD_SYSCLK)
#define	PX_SPD_33MHZ	0
#define	PX_SPD_40MHZ	1
#define	PX_SPD_50MHZ	2
#define	PX_SPD_66MHZ	3
#define	PX_SPD_83MHZ	4
#define	PX_SPD_100MHZ	5
#define	PX_SPD_133MHZ	6
#define	PX_SPD_166MHZ	7
#define	PX_SPD_DDRCLK	__BITS(3,5)
#define	PX_SPD_DDRCLK_GET(n)	__SHIFTOUT((n), PX_SPD_DDRCLK)
#define	PX_AUX2		0x08	/* Auxiliary 2 register */
#define	PX_CSR2		0x09	/* General control/status register 2 */
#define PX_CSR2_MMC_8BITEN	__BIT(7)
#define PX_CSR2_SDSEL_EN	__BIT(6)
#define PX_CSR2_MMC_8BIT	__BIT(4)
#define PX_CSR2_SDSEK		__BIT(3)
#define	PX_VWATCH	0x0a	/* VELA watchdog register */
#define	PX_LED		0x0b	/* LED data register */
#define	PX_PWR		0x0c	/* Power status register */
#define	PX_VCTRL	0x10	/* VELA control register */

#endif /* !_MPC85XX_PIXISREG_H_ */
