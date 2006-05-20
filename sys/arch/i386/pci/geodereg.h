/*	$NetBSD: geodereg.h,v 1.6.6.2 2006/05/20 12:25:40 tron Exp $	*/

/*-
 * Copyright (c) 2005 David Young.  All rights reserved.
 *
 * This code was written by David Young.
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
 *	This product includes software developed by David Young.
 * 4. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DAVID YOUNG ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL DAVID
 * YOUNG BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/*
 * Register definitions for the AMD Geode SC1100.
 */

#ifndef _I386_PCI_GEODEREG_H_
#define	_I386_PCI_GEODEREG_H_

/* Macros for bit twiddling. */

#ifndef _BIT_TWIDDLE
#define _BIT_TWIDDLE
/* nth bit, BIT(0) == 0x1. */
#define BIT(n) (((n) == 32) ? 0 : ((u_int32_t) 1 << (n)))

/* bits m through n, m < n. */
#define BITS(m, n) ((BIT(MAX((m), (n)) + 1) - 1) ^ (BIT(MIN((m), (n))) - 1))

/* find least significant bit that is set */
#define LOWEST_SET_BIT(x) ((((x) - 1) & (x)) ^ (x))

/* for x a power of two and p a non-negative integer, is x a greater power than 2**p? */
#define GTEQ_POWER(x, p) (((u_long)(x) >> (p)) != 0)

#define MASK_TO_SHIFT2(m) (GTEQ_POWER(LOWEST_SET_BIT((m)), 1) ? 1 : 0)

#define MASK_TO_SHIFT4(m) \
	(GTEQ_POWER(LOWEST_SET_BIT((m)), 2) \
	    ? 2 + MASK_TO_SHIFT2((m) >> 2) \
	    : MASK_TO_SHIFT2((m)))

#define MASK_TO_SHIFT8(m) \
	(GTEQ_POWER(LOWEST_SET_BIT((m)), 4) \
	    ? 4 + MASK_TO_SHIFT4((m) >> 4) \
	    : MASK_TO_SHIFT4((m)))

#define MASK_TO_SHIFT16(m) \
	(GTEQ_POWER(LOWEST_SET_BIT((m)), 8) \
	    ? 8 + MASK_TO_SHIFT8((m) >> 8) \
	    : MASK_TO_SHIFT8((m)))

#define MASK_TO_SHIFT(m) \
	(GTEQ_POWER(LOWEST_SET_BIT((m)), 16) \
	    ? 16 + MASK_TO_SHIFT16((m) >> 16) \
	    : MASK_TO_SHIFT16((m)))

#define MASK_AND_RSHIFT(x, mask) (((x) & (mask)) >> MASK_TO_SHIFT(mask))
#define LSHIFT(x, mask) ((x) << MASK_TO_SHIFT(mask))
#define MASK_AND_REPLACE(reg, val, mask) ((reg & ~mask) | LSHIFT(val, mask))
#define PRESHIFT(m) MASK_AND_RSHIFT((m), (m))

#endif /* _BIT_TWIDDLE */

/* AMD Geode SC1100 X-Bus PCI Configuration Register: General
 * Configuration Block Scratchpad.  Set to 0x00000000 after chip reset.
 * The BIOS writes the base address of the General Configuration
 * Block to this register.
 */
#define	SC1100_XBUS_CBA_SCRATCHPAD	0x64

#define	SC1100_GCB_SIZE			64

/* watchdog timeout register, 16 bits. */
#define	SC1100_GCB_WDTO			0x00	

/* Watchdog configuration register, 16 bits. */
#define	SC1100_GCB_WDCNFG		0x02
#define	SC1100_WDCNFG_RESERVED		BITS(15,9)	/* write as read */

/* 32kHz clock power-down, 0: clock is enabled, 1: clock is disabled. */
#define	SC1100_WDCNFG_WD32KPD		BIT(8)

/* Watchdog event type 1, and type 2
 *
 * 00: no action (default after POR# is asserted)
 * 01: interrupt
 * 10: SMI
 * 11: system reset
 */
#define	SC1100_WDCNFG_WDTYPE2_MASK	BITS(7,6)
#define	SC1100_WDCNFG_WDTYPE1_MASK	BITS(5,4)

#define	SC1100_WDCNFG_WDTYPE2_NOACTION	LSHIFT(0, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_INTERRUPT	LSHIFT(1, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_SMI	LSHIFT(2, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_RESET	LSHIFT(3, SC1100_WDCNFG_WDTYPE2_MASK)

#define	SC1100_WDCNFG_WDTYPE1_NOACTION	LSHIFT(0, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_INTERRUPT	LSHIFT(1, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_SMI	LSHIFT(2, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_RESET	LSHIFT(3, SC1100_WDCNFG_WDTYPE1_MASK)

/* Watchdog timer prescaler
 *
 * The prescaler divisor is 2**WDPRES.  1110 (0xe) and 1111 (0xf) are
 * reserved values.
 */
#define	SC1100_WDCNFG_WDPRES_MASK	BITS(3,0)
#define	SC1100_WDCNFG_WDPRES_MAX	0xd

/* Watchdog status register, 8 bits. */
#define	SC1100_GCB_WDSTS		0x04
#define	SC1100_WDSTS_RESERVED		BIT(7,4)	/* write as read */
/* Set to 1 when watchdog reset is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDRST		BIT(3)
/* Set to 1 when watchdog SMI is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDSMI		BIT(2)
/* Set to 1 when watchdog interrupt is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDINT		BIT(1)
/* Set to 1 when watchdog overflow is asserted.  Reset either by
 * POR# (power-on reset) or by writing writing 1 to this bit.
 */
#define	SC1100_WDSTS_WDOVF		BIT(0)

/*
 * Helpful constants
 */

/* maximum watchdog interval in seconds */
#define	SC1100_WDIVL_MAX	((1 << SC1100_WDCNFG_WDPRES_MAX) * \
				 UINT16_MAX / SC1100_WDCLK_HZ)
/* watchdog clock rate in Hertz */
#define	SC1100_WDCLK_HZ	32000

#endif /* _I386_PCI_GEODEREG_H_ */
