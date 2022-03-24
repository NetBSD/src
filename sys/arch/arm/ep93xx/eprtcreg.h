/*	$NetBSD: eprtcreg.h,v 1.2 2022/03/24 12:12:00 andvar Exp $	*/

/*
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*	Cirrus Logic EP9315
	RealTime Clock register
	http://www.cirrus.com/jp/pubs/manual/EP9315_Users_Guide.pdf	*/

#ifndef	_EPRTCREG_H_
#define	_EPRTCREG_H_

#define	EP93XX_RTC_Data		0x00	/* RTC Data Register (RO) */
#define	EP93XX_RTC_Match	0x04	/* RTC Match Register (R/W) */
#define	EP93XX_RTC_Sts		0x08	/* RTC Status/EOI Register (R/W) */
#define	 EP93XX_RTC_INTR	(1<<0)	/* Interrupt status */
#define	EP93XX_RTC_Load		0x0c	/* RTC Load Register (R/W) */
#define	EP93XX_RTC_Ctrl		0x10	/* RTC Control Register (R/W) */
#define	 EP93XX_RTC_MIE		(1<<0)	/* Match Interrupt Enable */
#define	EP93XX_RTC_SWComp	0x108	/* RTC Software Compensation (R/W) */
#define	 EP93XX_RTC_DEL_SHIFT	(1<<16)	/* Number of clocks to delete */
#define	 EP93XX_RTC_DEL_MASK	0x001f0000
#define	 EP93XX_RTC_INT_SHIFT	(1<<0)	/* Counter pre-load Integer value */
#define	 EP93XX_RTC_INT_MASK	0x0000ffff

#endif	/* _EPRTCREG_H_ */
