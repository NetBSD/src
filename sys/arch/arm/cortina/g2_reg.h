/* $NetBSD: g2_reg.h,v 1.1 2013/02/19 16:07:23 matt Exp $ */
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _ARM_CORTINA_G2_REG_H_
#define _ARM_CORTINA_G2_REG_H_

#define	G2_PERIPH_PBASE		0xf0000000
#define	G2_PERIPH_SIZE		0x00100000

#define	G2_SDRAM_PBASE		0xf0500000

#define	G2_TRNG_PBASE		0xf0600000

#define	G2_AHBAXI_PBASE		0xf4000000
#define	G2_EHCI_PBASE		0xf4000000
#define	G2_OHCI_PBASE		0xf4040000
#define	G2_USBD_PBASE		0xf4080000
#define	G2_SDC_PBASE		0xf40c0000
#define	G2_AHCI_PBASE		0xf4100000

#define	G2_RTC_PBASE		0xf4200000
#define	G2_CIR_PWRCTRL_PBASE	0xf4210000
#define	G2_SPDIF_PBASE		0xf4270000

#define	G2_ROMOTP_PBASE		0xf5008800

#define	G2_CRYPTO_PBASE		0xf6600000
#define	G2_CRYPTO_SIZE		0x00300000
#define	G2_RECIRC_TOP		0xf6800000

#define	G2_ARMCORE_PBASE	0xf8000000
#define	G2_ARMCORE_SIZE		0x00002000

#define	G2_PCIE0_PBASE		0x80000000
#define	G2_PCIE1_PBASE		0xa0000000
#define	G2_PCIE2_PBASE		0xc0000000
#defien	G2_PCIE_MEM_SIZE	0x20000000

#define	G2_UART0_BASE		0x70110
#define	G2_UART1_BASE		0x70140
#define	G2_UART2_BASE		0x70170
#define	G2_UART3_BASE		0x701a0

#define	G2_UART_CFG		0x00
#define	G2_UART_FC		0x04
#define	G2_UART_RXSAMPLE	0x08
#define	G2_UART_TXDAT		0x10
#define	G2_UART_RXDAT		0x14
#define	G2_UART_INFO		0x18
#define	G2_UART_IE0		0x1c
#define	G2_UART_IE1		0x20
#define	G2_UART_INT0		0x24
#define	G2_UART_INT1		0x28
#define	G2_UART_STAT		0x2c

#define	UART_CFG_BAUD_COUNT	__BITS(31,8)
#define	UART_CFG_UART_EN	__BIT(7)
#define	UART_CFG_RX_SM_EN	__BIT(6)
#define	UART_CFG_TX_SM_EN	__BIT(5)
#define	UART_CFG_PARITY_EN	__BIT(4)
#define	UART_CFG_EVEN_EN	__BIT(3)
#define	UART_CFG_STOP_2BIT	__BIT(2)
#define	UART_CFG_CHAR_CNT	__BITS(1,0)

#define	UART_FC_NO_RTS		__BIT(10)
#define	UART_FC_INV_RTS		__BIT(9)
#define	UART_FC_CTS_REG		__BIT(8)
#define	UART_FC_NO_CTS		__BIT(7)
#define	UART_FC_INV_CTS		__BIT(6)
#define	UART_FC_RX_WM		__BITS(5,0)

#define	UART_RXSAMPLE_CENTER	__BITS(23,0)

#define	UART_RXDAT_VALID	__BIT(8)
#define	UART_RXDAT_DATA		__BITS(7,0)

#define	UART_INFO_TXFIFO_EMPTY	__BIT(3)
#define	UART_INFO_TXFIFO_FULL	__BIT(2)
#define	UART_INFO_RXFIFO_EMPTY	__BIT(1)
#define	UART_INFO_RXFIFO_FULL	__BIT(0)

#define	UART_IE_RXFIFO_NONEMPTY	_BIT(6)
#define	UART_IE_TXFIFO_EMPTY	_BIT(5)
#define	UART_IE_RXFIFO_UNDERRUN	_BIT(4)
#define	UART_IE_RXFIFO_OVERRUN	_BIT(3)
#define	UART_IE_RXPARITY_ERR	_BIT(2)
#define	UART_IE_RXSTOP_ERR	_BIT(1)
#define	UART_IE_TXFIFO_OVERRUN	_BIT(0)

#endif /* _ARM_CORTINA_G2_REG_H_ */
