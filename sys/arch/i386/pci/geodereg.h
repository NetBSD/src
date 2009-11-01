/*	$NetBSD: geodereg.h,v 1.8.74.1 2009/11/01 13:58:35 jym Exp $	*/

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

#include <lib/libkern/libkern.h>

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
#define	SC1100_WDCNFG_RESERVED		__BITS(15,9)	/* write as read */

/* 32kHz clock power-down, 0: clock is enabled, 1: clock is disabled. */
#define	SC1100_WDCNFG_WD32KPD		__BIT(8)

/* Watchdog event type 1, and type 2
 *
 * 00: no action (default after POR# is asserted)
 * 01: interrupt
 * 10: SMI
 * 11: system reset
 */
#define	SC1100_WDCNFG_WDTYPE2_MASK	__BITS(7,6)
#define	SC1100_WDCNFG_WDTYPE1_MASK	__BITS(5,4)

#define	SC1100_WDCNFG_WDTYPE2_NOACTION	__SHIFTIN(0, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_INTERRUPT	__SHIFTIN(1, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_SMI	__SHIFTIN(2, SC1100_WDCNFG_WDTYPE2_MASK)
#define	SC1100_WDCNFG_WDTYPE2_RESET	__SHIFTIN(3, SC1100_WDCNFG_WDTYPE2_MASK)

#define	SC1100_WDCNFG_WDTYPE1_NOACTION	__SHIFTIN(0, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_INTERRUPT	__SHIFTIN(1, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_SMI	__SHIFTIN(2, SC1100_WDCNFG_WDTYPE1_MASK)
#define	SC1100_WDCNFG_WDTYPE1_RESET	__SHIFTIN(3, SC1100_WDCNFG_WDTYPE1_MASK)

/* Watchdog timer prescaler
 *
 * The prescaler divisor is 2**WDPRES.  1110 (0xe) and 1111 (0xf) are
 * reserved values.
 */
#define	SC1100_WDCNFG_WDPRES_MASK	__BITS(3,0)
#define	SC1100_WDCNFG_WDPRES_MAX	0xd

/* Watchdog status register, 8 bits. */
#define	SC1100_GCB_WDSTS		0x04
#define	SC1100_WDSTS_RESERVED		__BIT(7,4)	/* write as read */
/* Set to 1 when watchdog reset is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDRST		__BIT(3)
/* Set to 1 when watchdog SMI is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDSMI		__BIT(2)
/* Set to 1 when watchdog interrupt is asserted.  Read-only.  Reset either by
 * POR# (power-on reset) or by writing 0 to WDOVF.
 */
#define	SC1100_WDSTS_WDINT		__BIT(1)
/* Set to 1 when watchdog overflow is asserted.  Reset either by
 * POR# (power-on reset) or by writing 1 to this bit.
 */
#define	SC1100_WDSTS_WDOVF		__BIT(0)

/*
 * Helpful constants
 */

/* maximum watchdog interval in seconds */
#define	SC1100_WDIVL_MAX	((1 << SC1100_WDCNFG_WDPRES_MAX) * \
				 UINT16_MAX / SC1100_WDCLK_HZ)
/* watchdog clock rate in Hertz */
#define	SC1100_WDCLK_HZ	32000

/*
 * high resolution timer
 */
#define SC1100_GCB_TMVALUE_L		0x08    /* timer value */

#define SC1100_GCB_TMSTS_B		0x0c    /* status */
#define SC1100_TMSTS_OVFL		__BIT(0)  /* set: overflow */

#define SC1100_GCB_TMCNFG_B		0x0d    /* configuration */
#define SC1100_TMCNFG_TM27MPD		__BIT(2)  /* set: power down on SUSPA# */
#define SC1100_TMCNFG_TMCLKSEL		__BIT(1)  /* set: 27MHz clock, clear: 1MHz */
#define SC1100_TMCNFG_TMEN		__BIT(0)  /* set: timer interrupt enabled */

#define SC1100_GCB_IID_B		0x3c    /* chip identification register */

#define SC1100_GCB_REV_B		0x3d    /* revision register */

#endif /* _I386_PCI_GEODEREG_H_ */
