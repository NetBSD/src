/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_MPCORE_MPCOREREG_H
#define _ARM_MPCORE_MPCOREREG_H

/*
 * Private Memory Region
 */
#define	MPCORE_PMR_SCU 	0x0000
#define	MPCORE_PMR_CII 	0x0100	/* CPU Interrupt Interface (for current core) */
#define	MPCORE_PMR_CII_CORE(n)	(0x0x200 + 0x100 * (n))	/* for core N */
#define	MPCORE_PMR_CII_SIZE	0x100
#define	MPCORE_PMR_TIMER	0x600	/* for current core */
#define	MPCORE_PMR_TIMER_CORE(n)	(0x700 + 0x100 * (n))	/* for core N */
#define	MPCORE_PMR_TIMER_SIZE	0x10
#define	MPCORE_PMR_WDOG 	0x620
#define	MPCORE_PMR_WDOG_SIZE	0x18
#define	MPCORE_PMR_GID 	0x1000	/* Global Interrupt Distributor */
#define	MPCORE_PMR_GID_SIZE	0x1000

#define	MPCORE_PMR_SIZE	0x2000


/*
 * CPU internal timer and watchdog
 */

#define	PMR_CLK_LOAD	0x00
#define	PMR_CLK_COUNTER	0x04
#define	PMR_CLK_CONTROL	0x08
#define	 CLK_CONTROL_ENABLE	__BIT(0)
#define	 CLK_CONTROL_AUTOLOAD	__BIT(1)
#define	 CLK_CONTROL_ITENABLE	__BIT(2)
#define	 CLK_CONTROL_PRESCALER_SHIFT	8
#define	 CLK_CONTROL_PRESCALER_MASK	__BITS(CLK_CONTROL_PRESCALER_SHIFT, 15)

#define	PMR_CLK_INTR	0x0c

#define	PMR_WDOG_LOAD		0x00
#define	PMR_WDOG_COUNTER	0x04
#define	PMR_WDOG_COUNTER	0x04
#define	PMR_WDOG_CONTROL	0x08
#define	 WDOG_CONTROL_ENABLE	__BIT(0)
#define	 WDOG_CONTROL_AUTOLOAD	__BIT(1)
#define	 WDOG_CONTROL_ITENABLE	__BIT(2)
#define	 WDOG_CONTROL_MODE	__BIT(3)
#define	 WDOG_CONTROL_PRESCALER_SHIFT	8
#define	 WDOG_CONTROL_PRESCALER_MASK	__BITS(WDOG_CONTROL_PRESCALER_SHIFT, 15)

#define	PMR_WDOG_INTR   	0x0c
#define	PMR_WDOG_RESET   	0x10
#define	PMR_WDOG_DISABLE   	0x14

#endif	/* _ARM_MPCORE_MPCOREREG_H */
