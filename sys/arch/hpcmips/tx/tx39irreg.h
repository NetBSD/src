/*	$NetBSD: tx39irreg.h,v 1.2 2001/06/14 11:09:56 uch Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * Toshiba TX3912 IR module
 */

#define TX39_IRCTRL1_REG	0x0a0 /* R/W */
#define TX39_IRCTRL2_REG	0x0a4 /* W */
#define TX39_IRTXHOLD_REG	0x0a8 /* W */

/*
 * IR control 1 register
 */
#define TX39_IRCTRL1_CARDET	0x01000000

#define TX39_IRCTRL1_BAUDVAL_SHIFT	16
#define TX39_IRCTRL1_BAUDVAL_MASK	0xff
#define TX39_IRCTRL1_BAUDVAL(cr)					\
	(((cr) >> TX39_IRCTRL1_BAUDVAL_SHIFT) &				\
	TX39_IRCTRL1_BAUDVAL_MASK)
#define TX39_IRCTRL1_BAUDVAL_SET(cr, val)				\
	((cr) | (((val) << TX39_IRCTRL1_BAUDVAL_SHIFT) &		\
	(TX39_IRCTRL1_BAUDVAL_MASK << TX39_IRCTRL1_BAUDVAL_SHIFT)))
#define TX39_IRCTRL1_BAUDVAL_CLR(cr)					\
	((cr) &= ~(TX39_IRCTRL1_BAUDVAL_MASK << TX39_IRCTRL1_BAUDVAL_SHIFT))

#define TX39_IRCTRL1_TESTIR	0x00000010 /* don't set */
#define TX39_IRCTRL1_DTINVERT	0x00000008
#define TX39_IRCTRL1_RXPWR	0x00000004
#define TX39_IRCTRL1_ENSTATE	0x00000002
#define TX39_IRCTRL1_ENCOMSM	0x00000001

/*
 * IR control 2 register
 */
/*
 * period = (PER + 1) * (BAUDVAL + 1) * (1/3.6864MHz)
 */
#define TX39_IRCTRL2_PER_SHIFT		24
#define TX39_IRCTRL2_PER_MASK		0xff
#define TX39_IRCTRL2_PER_SET(cr, val)					\
	((cr) | (((val) << TX39_IRCTRL2_PER_SHIFT) &			\
	(TX39_IRCTRL2_PER_MASK << TX39_IRCTRL2_PER_SHIFT)))
#define TX39_IRCTRL2_PER_CLR(cr)					\
	((cr) &= ~(TX39_IRCTRL2_PER_MASK << TX39_IRCTRL2_PER_SHIFT))

/*
 * on time = ONTIME * (BAUDVAL + 1) * (1/3.6864MHz)
 */
#define TX39_IRCTRL2_ONTIME_SHIFT	16
#define TX39_IRCTRL2_ONTIME_MASK	0xff
#define TX39_IRCTRL2_ONTIME_SET(cr, val)				\
	((cr) | (((val) << TX39_IRCTRL2_ONTIME_SHIFT) &			\
	(TX39_IRCTRL2_ONTIME_MASK << TX39_IRCTRL2_ONTIME_SHIFT)))
#define TX39_IRCTRL2_ONTIME_CLR(cr)					\
	((cr) &= ~(TX39_IRCTRL2_ONTIME_MASK << TX39_IRCTRL2_ONTIME_SHIFT))

/*
 * delay time = (DELAYVAL + 1) * 7.8ms
 */
#define TX39_IRCTRL2_DELAYVAL_SHIFT	8
#define TX39_IRCTRL2_DELAYVAL_MASK	0xff
#define TX39_IRCTRL2_DELAYVAL_SET(cr, val)				\
	((cr) | (((val) << TX39_IRCTRL2_DELAYVAL_SHIFT) &		\
	(TX39_IRCTRL2_DELAYVAL_MASK << TX39_IRCTRL2_DELAYVAL_SHIFT)))
#define TX39_IRCTRL2_DELAYVAL_CLR(cr)					\
	((cr) &= ~(TX39_IRCTRL2_DELAYVAL_MASK << TX39_IRCTRL2_DELAYVAL_SHIFT))

/*
 * wait time = (DELAYVAL + 1) * (WAITVAL + 1) * 7.8ms
 */
#define TX39_IRCTRL2_WAITVAL_SHIFT	0
#define TX39_IRCTRL2_WAITVAL_MASK	0xff
#define TX39_IRCTRL2_WAITVAL_SET(cr, val)				\
	((cr) | (((val) << TX39_IRCTRL2_WAITVAL_SHIFT) &		\
	(TX39_IRCTRL2_WAITVAL_MASK << TX39_IRCTRL2_WAITVAL_SHIFT)))
#define TX39_IRCTRL2_WAITVAL_CLR(cr)					\
	((cr) &= ~(TX39_IRCTRL2_WAITVAL_MASK << TX39_IRCTRL2_WAITVAL_SHIFT))
