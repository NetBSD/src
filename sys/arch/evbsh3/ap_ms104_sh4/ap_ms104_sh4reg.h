/*	$NetBSD: ap_ms104_sh4reg.h,v 1.1.18.1 2012/02/18 07:32:00 mrg Exp $	*/

/*-
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	AP_MS104_SH4REG_H_
#define	AP_MS104_SH4REG_H_

#define	EXTINTR_MASK1		0xa4000000	/* R/W: 8bit */
#define	EXTINTR_MASK2		0xa4100000	/* R/W: 8bit */
#define	EXTINTR_MASK3		0xa4200000	/* R/W: 8bit */
#define	EXTINTR_MASK4		0xa4300000	/* R/W: 8bit */
#define	EXTINTR_STAT1		0xa4400000	/* R: 8bit */
#define	EXTINTR_STAT2		0xa4500000	/* R: 8bit */
#define	EXTINTR_STAT3		0xa4600000	/* R: 8bit */
#define	EXTINTR_STAT4		0xa4700000	/* R: 8bit */
#define	CFBUS_CTRL		0xa4800000	/* W: 8bit */

/* EXTINTR_MASK1 */
#define	MASK1_INT14		(1U << 0)
#define	MASK1_INT13		(1U << 1)
#define	MASK1_INT12		(1U << 2)
#define	MASK1_INT11		(1U << 3)

/* EXTINTR_MASK2 */
#define	MASK2_INT10		(1U << 0)
#define	MASK2_INT9		(1U << 1)
#define	MASK2_INT8		(1U << 2)
#define	MASK2_INT7		(1U << 3)

/* EXTINTR_MASK3 */
#define	MASK3_INT6		(1U << 0)
#define	MASK3_INT5		(1U << 1)
#define	MASK3_INT4		(1U << 2)
#define	MASK3_INT3		(1U << 3)

/* EXTINTR_MASK4 */
#define	MASK4_INT2		(1U << 0)
#define	MASK4_INT1		(1U << 1)

/* EXTINTR_STAT1 */
#define	STAT1_INT14		(1U << 0)
#define	STAT1_INT13		(1U << 1)
#define	STAT1_INT12		(1U << 2)
#define	STAT1_INT11		(1U << 3)

/* EXTINTR_STAT2 */
#define	STAT2_INT10		(1U << 0)
#define	STAT2_INT9		(1U << 1)
#define	STAT2_INT8		(1U << 2)
#define	STAT2_INT7		(1U << 3)

/* EXTINTR_STAT3 */
#define	STAT3_INT6		(1U << 0)
#define	STAT3_INT5		(1U << 1)
#define	STAT3_INT4		(1U << 2)
#define	STAT3_INT3		(1U << 3)

/* EXTINTR_STAT4 */
#define	STAT4_INT2		(1U << 0)
#define	STAT4_INT1		(1U << 1)

/* CFBUS_CTRL */
#define	CFBUS_CTRL_WAIT		(1U << 0)
#define	CFBUS_CTRL_IOIS16	(1U << 1)

/* external intr# */
#define	EXTINTR_INTR_SMC91C111	8
#define	EXTINTR_INTR_CFIREQ	12
#define	EXTINTR_INTR_RTC	14

/* GPIO pin# */
#define	GPIO_PIN_CARD_CD	8	/* In */
#define	GPIO_PIN_CARD_PON	9	/* Out */
#define	GPIO_PIN_CARD_RESET	10	/* Out */
#define	GPIO_PIN_CARD_ENABLE	11	/* Out */
#define	GPIO_PIN_RTC_SIO	13	/* In/Out */
#define	GPIO_PIN_RTC_SCLK	14	/* Out */
#define	GPIO_PIN_RTC_CE		15	/* Out */

#endif	/* AP_MS104_SH4REG_H_ */
