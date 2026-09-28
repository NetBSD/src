/*	$NetBSD: samfpgareg.h,v 1.1 2026/09/28 20:41:00 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * FPGA chip on ACube Sam460ex (Lattice XP2).
 */

#ifndef _EVBPPC_SAM460EX_SAMFPGAREG_H_
#define _EVBPPC_SAM460EX_SAMFPGAREG_H_

#define	SAMFPGA_SIZE		0x40

/* J22 GPIO */
#define	SAMFPGA_GPIO_NBANK	5
#define	SAMFPGA_GPIO_NPINS	80
#define	SAMFPGA_GPIO_OUT(b)	(0x02 + 2 * (b))	/* output, r/w */
#define	SAMFPGA_GPIO_DIR(b)	(0x0c + 2 * (b))	/* 1 = output */
#define	SAMFPGA_GPIO_IN(b)	(0x20 + 2 * (b))	/* pin level */
#define	SAMFPGA_GPIO_BANK(p)	((p) >> 4)
#define	SAMFPGA_GPIO_BIT(p)	__BIT((p) & 0xf)

/* FPGA design revision, BCD */
#define	SAMFPGA_REV_DATE	0x2a
#define	 SAMFPGA_REV_DATE_DAY	__BITS(15, 8)
#define	 SAMFPGA_REV_DATE_MONTH	__BITS(7, 0)
#define	SAMFPGA_REV_YEAR	0x2c
#define	 SAMFPGA_REV_YEAR_YEAR	__BITS(15, 8)	/* 20xx */
#define	 SAMFPGA_REV_YEAR_REV	__BITS(7, 0)

/* Board control */
#define	SAMFPGA_CTL		0x2e
#define	 SAMFPGA_CTL_B0		__BIT(0)	/* ??? */
#define	 SAMFPGA_CTL_LED_RED	__BIT(1)	/* 1 = on */
#define	 SAMFPGA_CTL_LED_YELLOW	__BIT(2)
#define	 SAMFPGA_CTL_LED_AMBER	__BIT(3)
#define	 SAMFPGA_CTL_LEDS	__BITS(3, 1)
#define	 SAMFPGA_CTL_RESET	__BIT(4)	/* pulsed: board reset; held: power off */

#define	SAMFPGA_USB		0x30		/* ??? U-Boot sets bit 2 */

#endif	/* _EVBPPC_SAM460EX_SAMFPGAREG_H_ */
