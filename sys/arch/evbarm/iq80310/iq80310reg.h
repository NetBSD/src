/*	$NetBSD: iq80310reg.h,v 1.6 2005/12/24 20:06:59 perry Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IQ80310REG_H_
#define	_IQ80310REG_H_

/*
 * Memory map and register definitions for the Intel IQ80310
 * Evaluation Board.
 */

/*
 * The memory map of the IQ80310 looks like so:
 *
 *           ------------------------------
 *		On-board devices
 *		Flash Bank 0
 * FE80 0000 ------------------------------
 *		DRAM
 * A000 0000 ------------------------------
 *		Reserved
 * 9002 0000 ------------------------------
 *		ATU Outbound Transaction
 *		    Windows
 * 8000 0000 ------------------------------
 *		ATU Outbound Direct
 *		    Addressing Windows
 * 0080 0000 ------------------------------
 *		Flash Bank 1
 * 0000 2000 ------------------------------
 *		Reserved
 * 0000 1900 ------------------------------
 *		Peripheral Memory Mapped
 *		    Registers
 * 0000 1000 ------------------------------
 *		Initialization Boot Code
 *		    from Flash Bank 1
 * 0000 0000 ------------------------------
 */

/*
 * We map the CPLD registers VA==PA, so we go ahead and cheat
 * with register access.
 */
#define	CPLD_READ(x)		*((volatile uint8_t *)(x))
#define	CPLD_WRITE(x, v)	*((volatile uint8_t *)(x)) = (v)

/*
 * We allocate a page table for VA 0xfe400000 (4MB) and map the i80312
 * PCI I/O space (2 * 64L) and i80312 regisers (4K) there.
 */
#define	IQ80310_IOPXS_VBASE	0xfe400000UL
#define	IQ80310_PIOW_VBASE	IQ80310_IOPXS_VBASE
#define	IQ80310_SIOW_VBASE	(IQ80310_PIOW_VBASE + I80312_PCI_XLATE_IOSIZE)
#define	IQ80310_80312_VBASE	(IQ80310_SIOW_VBASE + I80312_PCI_XLATE_IOSIZE)

/*
 * The IQ80310 on-board devices are mapped VA==PA during bootstrap.
 * Conveniently, the size of the on-board register space is 1 section
 * mapping.
 */
#define	IQ80310_OBIO_BASE	0xfe800000UL
#define	IQ80310_OBIO_SIZE	0x00100000UL	/* 1MB */

#define	IQ80310_UART1		0xfe800000UL	/* XR 16550 */

#define	IQ80310_UART2		0xfe810000UL	/* XR 16550 */

#define	IQ80310_XINT3_STATUS	0xfe820000UL
#define	XINT3_TIMER		0		/* CPLD timer */
#define	XINT3_ETHERNET		1		/* on-board i82559 */
#define	XINT3_UART1		2		/* 16550 #1 */
#define	XINT3_UART2		3		/* 16550 #2 */
#define	XINT3_SINTD		4		/* INTD# */
#define	XINT3_BIT(x)		(1U << (x))

#define	IQ80310_BOARD_REV	0xfe830000UL	/* rev F and later (??) */
#define	BOARD_REV(x)		(((x) & 0xf) + '@')

#define	IQ80310_CPLD_REV	0xfe840000UL
#define	CPLD_REV(x)		(((x) & 0xf) + '@')

#define	IQ80310_7SEG_MSB	0xfe840000UL
#define	IQ80310_7SEG_LSB	0xfe850000UL

#define	IQ80310_XINT0_STATUS	0xfe850000UL	/* rev F and later */
#define	XINT0_SINTA		0		/* INTA# */
#define	XINT0_SINTB		1		/* INTB# */
#define	XINT0_SINTC		2		/* INTC# */
#define	XINT0_BIT(x)		(1U << (x))

#define	IQ80310_XINT_MASK	0xfe860000UL
	/* See XINT_STATUS bits: 0 == int enabled, 1 == int disabled */

#define	IQ80310_BACKPLANE_DET	0xfe870000UL

#define	IQ80310_TIMER_LA0	0xfe880000UL

#define	IQ80310_TIMER_LA1	0xfe890000UL

#define	IQ80310_TIMER_LA2	0xfe8a0000UL

#define	IQ80310_TIMER_LA3	0xfe8b0000UL

#define	IQ80310_TIMER_ENABLE	0xfe8c0000UL
#define	TIMER_ENABLE_EN		(1U << 0)	/* enable counter */
#define	TIMER_ENABLE_INTEN	(1U << 1)	/* enable interrupt */

#define	IQ80310_ROT_SWITCH	0xfe8d0000UL

#define	IQ80310_JTAG		0xfe8e0000UL

#define	IQ80310_BATTERY_STAT	0xfe8f0000UL
#define	BATTERY_STAT_PRES	(1U << 0)
#define	BATTERY_STAT_CHRG	(1U << 1)
#define	BATTERY_STAT_DISCHRG	(1U << 2)
#define	BATTERY_STAT_PWRDELAY	(1U << 3)	/* rev F and later */

#endif /* _IQ80310REG_H_ */
