/*	$NetBSD: sa11x0_gpioreg.h,v 1.9.2.1 2001/08/03 04:11:34 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * SA-11x0 GPIO Register 
 */

#define SAGPIO_NPORTS	8

/* GPIO pin-level register */
#define SAGPIO_PLR	0x00

/* GPIO pin direction register */
#define SAGPIO_PDR	0x04

/* GPIO pin output set register */
#define SAGPIO_PSR	0x08

/* GPIO pin output clear register */
#define SAGPIO_PCR	0x0C

/* GPIO rising-edge detect register */
#define SAGPIO_RER	0x10

/* GPIO falling-edge detect register */
#define SAGPIO_FER	0x14

/* GPIO edge-detect status register */
#define SAGPIO_EDR	0x18

/* GPIO alternate function register */
#define SAGPIO_AFR	0x1C

/* XXX */
#define GPIO(x)		(0x00000001 << (x))

/*
 * SA-11x0 GPIOs parameter
 *	Alternate Functions
 */
#define GPIO_ALT_LDD8		GPIO(2)		/* LCD DATA(8) */
#define GPIO_ALT_LDD9		GPIO(3)		/* LCD DATA(9) */
#define GPIO_ALT_LDD10		GPIO(4)		/* LCD DATA(10) */
#define GPIO_ALT_LDD11		GPIO(5)		/* LCD DATA(11) */
#define GPIO_ALT_LDD12		GPIO(6)		/* LCD DATA(12) */
#define GPIO_ALT_LDD13		GPIO(7)		/* LCD DATA(13) */
#define GPIO_ALT_LDD14		GPIO(8)		/* LCD DATA(14) */
#define GPIO_ALT_LDD15		GPIO(9)		/* LCD DATA(15) */
#define GPIO_ALT_SSP_TXD	GPIO(10)	/* SSP transmit */
#define GPIO_ALT_SSP_RXD	GPIO(11)	/* SSP receive */
#define GPIO_ALT_SSP_SCLK	GPIO(12)	/* SSP serial clock */
#define GPIO_ALT_SSP_SFRM	GPIO(13)	/* SSP frameclock */
#define GPIO_ALT_UART_TXD	GPIO(14)	/* UART transmit */
#define GPIO_ALT_UART_RXD	GPIO(15)	/* UART receive */
#define GPIO_ALT_GPCLK_OUT	GPIO(16)	/* General-purpose clock out */
#define GPIO_ALT_UART_SCLK	GPIO(18)	/* Sample clock input */
#define GPIO_ALT_SSP_CLK	GPIO(19)	/* Sample clock input */
#define GPIO_ALT_UART_SCLK3	GPIO(20)	/* Sample clock input */
#define GPIO_ALT_MCP_CLK	GPIO(21)	/* MCP dock in */
#define GPIO_ALT_TREQA		GPIO(22)	/* Either TIC request A */
#define GPIO_ALT_TREQB		GPIO(23)	/* Either TIC request B */
#define GPIO_ALT_RTC		GPIO(25)	/* Real Time Clock */
#define GPIO_ALT_RCLK_OUT	GPIO(26)	/* internal clock /2 */
#define GPIO_ALT_32KHZ_OUT	GPIO(27)	/* Raw 32.768kHz osc output */
