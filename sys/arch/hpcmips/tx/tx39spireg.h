/*	$NetBSD: tx39spireg.h,v 1.2 2001/06/14 11:09:56 uch Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Toshiba TX3912/3922 SPI module
 */

#define TX39_SPICTRL_REG	0x160
#define TX39_SPITXHOLD_REG	0x164 /* W */
#define TX39_SPIRXHOLD_REG	0x164 /* R */

/*
 *	SPI Control Register
 */
/* R */
#define TX39_SPICTRL_SPION		0x00020000
#define TX39_SPICTRL_EMPTY		0x00010000
/* R/W */
#define TX39_SPICTRL_DELAYVAL_SHIFT	12
#define TX39_SPICTRL_DELAYVAL_MASK	0xf
#define TX39_SPICTRL_DELAYVAL(cr)					\
	(((cr) >> TX39_SPICTRL_DELAYVAL_SHIFT) &			\
	TX39_SPICTRL_DELAYVAL_MASK)
#define TX39_SPICTRL_DELAYVAL_SET(cr, val)				\
	((cr) | (((val) << TX39_SPICTRL_DELAYVAL_SHIFT) &		\
	(TX39_SPICTRL_DELAYVAL_MASK << TX39_SPICTRL_DELAYVAL_SHIFT)))
/* SPICLK Rate = 7.3728MHz/(BAUDRATE*2 + 2) */
#define TX39_SPICTRL_BAUDRATE_SHIFT	8
#define TX39_SPICTRL_BAUDRATE_MASK	0xf
#define TX39_SPICTRL_BAUDRATE(cr)					\
	(((cr) >> TX39_SPICTRL_BAUDRATE_SHIFT) &			\
	TX39_SPICTRL_BAUDRATE_MASK)
#define TX39_SPICTRL_BAUDRATE_SET(cr, val)				\
	((cr) | (((val) << TX39_SPICTRL_BAUDRATE_SHIFT) &		\
	(TX39_SPICTRL_BAUDRATE_MASK << TX39_SPICTRL_BAUDRATE_SHIFT)))
#define TX39_SPICTRL_PHAPOL	       0x00000020
#define TX39_SPICTRL_CLKPOL	       0x00000010
#define TX39_SPICTRL_WORD	       0x00000004
#define TX39_SPICTRL_LSB	       0x00000002
#define TX39_SPICTRL_ENSPI	       0x00000001

/*
 *	SPI Transmitter Holding Register
 */
/* W */
#define TX39_SPICTRL_TXDATA_SHIFT	0
#define TX39_SPICTRL_TXDATA_MASK	0xffff
#define TX39_SPICTRL_TXDATA_SET(cr, val)				\
	((cr) | (((val) << TX39_SPICTRL_TXDATA_SHIFT) &			\
	(TX39_SPICTRL_TXDATA_MASK << TX39_SPICTRL_TXDATA_SHIFT)))

/*
 *	SPI Receiver Holding Register
 */
/* R */
#define TX39_SPICTRL_RXDATA_SHIFT	8
#define TX39_SPICTRL_RXDATA_MASK	0xf
#define TX39_SPICTRL_RXDATA(cr)						\
	(((cr) >> TX39_SPICTRL_RXDATA_SHIFT) &				\
	TX39_SPICTRL_RXDATA_MASK)
