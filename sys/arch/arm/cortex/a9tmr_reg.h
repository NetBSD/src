/*	$NetBSD: a9tmr_reg.h,v 1.1 2012/09/01 00:03:14 matt Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * ARM MPCORE Global Timer Register Definitions
 *
 * These registers are accessible through a dedicated internal bus.
 * All accesses must be done in a little-endian manner.
 * The base address of the pages containing these registers is defined
 * by the pins PERIPHBASE[31:13] which can be obtained by doing a
 *	MRC p15,4,<Rd>,c15,c0,0; Read Configuration Base Address Register
 *	(except cortex-A9 uniprocessor)
 *
 */

#ifndef _ARM_CORTEX_A9TMR_REG_H_
#define	_ARM_CORTEX_A9TMR_REG_H_

#define	TMR_GLOBAL_BASE		0x0200	// Offset in PeriphBase
#define	TMR_PRIVATE_BASE	0x0600
#define	TMR_WDOG_BASE		0x0620
#define	TMR_GLOBAL_SIZE		0x0100
#define	TMR_PRIVATE_SIZE	0x0020
#define	TMR_WDOG_SIZE		0x0020

/*
 * F(timer) = PeriphClk / ((PreScaler_Value + 1) * Load_Value + 1))
 */
#define	TMR_LOAD		0x0000	// Timer Load Register
#define	TMR_CTR			0x0004	// Timer Counter Register
#define	TMR_CTL			0x0008	// Timer Control Register
#define	TMR_INT			0x000C	// Timer Interrupt Status
#define	TMR_RST			0x0010  // Timer Reset Status (WDOG only)
#define	TMR_WDOGDIS		0x0014  // [WO] Timer Disable (WDOG only)

#define	TMR_CTL_PRESCALER	__BITS(15,8)
#define	TMR_CTL_WDOG_MODE	__BIT(3) // WDOG mode
#define	TMR_CTL_INT_ENABLE	__BIT(2) // INT 29/30 is enabled
#define	TMR_CTL_AUTO_RELOAD	__BIT(1)
#define	TMR_CTL_ENABLE		__BIT(0)

#define	TMR_INT_EVENT		__BIT(0) // [W1C] timer reached 0
#define	TMR_RST_EVENT		__BIT(0) // [W1C] wdog timer reached 0

#define	TMR_WDOG_DISABLE_MAGIC1	0x12345678
#define	TMR_WDOG_DISABLE_MAGIC2	0x87654321

/*
 * Global Timer is a 64-bit incrementing counter.  As much as we'd like to
 * be able to use LDRD for loading the 64-bit counter, we aren't allowed to.
 */
#define	TMR_GBL_CTR_L		0x000 // Global Timer 64-bit Lower Value
#define	TMR_GBL_CTR_U		0x004 // Global Timer 64-bit Upper Timer
#define	TMR_GBL_CTL		0x008 // Global Timer Control
#define	TMR_GBL_INT		0x00c // [L] Global Timer Interrupt Status
#define	TMR_GBL_CMP_L		0x010 // [L] Global Timer 64-bit Comparator Low
#define	TMR_GBL_CMP_H		0x014 // [L] Global Timer 64-bit Comparator High
#define	TMR_GBL_AUTOINC		0x018 // [L] Global Timer Auto-Increment

#define	TMR_GBL_CTL_PRESCALER	__BIT(15,8)
#define	TMR_GBL_CTL_AUTO_INC	__BIT(3) // Auto Increment is enabled
#define	TMR_GBL_CTL_INT_ENABLE	__BIT(2) // [banked] INT 27 is enabled
#define	TMR_GBL_CTL_CMP_ENABLE	__BIT(1) // [banked] 
#define	TMR_GBL_CTL_ENABLE	__BIT(0)

#endif /* !_ARM_CORTEX_A9TMR_REG_H_ */
