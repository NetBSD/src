/* $NetBSD: lpssreg.h,v 1.1 2017/12/10 17:12:54 bouyer Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

/* LPSS registers, as found in several functions of Ivy Lake bridge */
#define LPSS_RESET	0x204
#define LPSS_RESET_DMA		__BIT(2)
#define LPSS_RESET_CTRL		__BITS(1,0)
#define LPSS_RESET_CTRL_ASSERT		0
#define LPSS_RESET_CTRL_REL		0x3

#define LPSS_ACTIVELTR	0x210
#define LPSS_ACTIVELTR_SSCALE	__BITS(12,10)
#define LPSS_ACTIVELTR_SSCALE_1		(0x2 << 10)
#define LPSS_ACTIVELTR_SSCALE_32	(0x3 << 10)
#define LPSS_ACTIVELTR_SVALUE	__BITS(9,0)

#define LPSS_IDLELTR	0x214
#define LPSS_IDLELTR_SSCALE	__BITS(12,10)
#define LPSS_IDLELTR_SSCALE_1		(0x2 << 10)
#define LPSS_IDLELTR_SSCALE_32		(0x3 << 10)
#define LPSS_IDLELTR_SVALUE	__BITS(9,0)

#define LPSS_TXACK	0x218
#define LPSS_TXACK_OVF		__BIT(31)
#define LPSS_TXACK_CNT		__BITS(23,0)

#define LPSS_RXACK	0x21C
#define LPSS_RXACK_OVF		__BIT(31)
#define LPSS_RXACK_CNT		__BITS(23,0)

#define LPSS_TX_IRQ	0x220
#define LPSS_TX_IRQ_MSK		__BIT(1)
#define LPSS_TX_IRQ_I		__BIT(0)

#define LPSS_TX_IRQ_CLR	0x224
#define LPSS_TX_IRQ_CLRI	__BIT(0)

#define LPSS_CLKGATE	0x238
#define LPSS_CLKGATE_DMA	__BITS(3,2)
#define LPSS_CLKGATE_DMA_AUTO		(0x0 << 2)
#define LPSS_CLKGATE_DMA_OFF		(0x2 << 2)
#define LPSS_CLKGATE_DMA_ON		(0x3 << 2)
#define LPSS_CLKGATE_CTRL	__BITS(1,0)
#define LPSS_CLKGATE_CTRL_AUTO		(0x0 << 0)
#define LPSS_CLKGATE_CTRL_OFF		(0x2 << 0)
#define LPSS_CLKGATE_CTRL_ON		(0x3 << 0)

#define LPSS_REMAP_LO	0x240

#define LPSS_REMAP_HI	0x244

#define LPSS_DEVIDLE	0x24c
#define LPSS_DEVIDLE_IRQ	__BIT(4)
#define LPSS_DEVIDLE_RESTORE	__BIT(3)
#define LPSS_DEVIDLE_IDLE	__BIT(2)
#define LPSS_DEVIDLE_INPROG	__BIT(0)

#define LPSS_CAP	0x2fc
#define LPSS_CAP_DMA		__BIT(8)
#define LPSS_CAP_TYPE		__BITS(7,4)
#define LPSS_CAP_TYPE_I2C		(0x0 << 4)
#define LPSS_CAP_TYPE_UART		(0x1 << 4)
#define LPSS_CAP_TYPE_SPI		(0x2 << 4)
#define LPSS_CAP_INSTANCE	__BITS(3,0)
