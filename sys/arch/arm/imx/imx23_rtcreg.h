/* $Id: imx23_rtcreg.h,v 1.1.6.2 2013/02/25 00:28:27 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_RTCREG_H_
#define _ARM_IMX_IMX23_RTCREG_H_

#include <sys/cdefs.h>

#define HW_RTC_BASE 0x8005C000

/*
 * Real-Time Clock Control Register.
 */
#define HW_RTC_CTRL	0x000
#define HW_RTC_CTRL_SET	0x004
#define HW_RTC_CTRL_CLR	0x008
#define HW_RTC_CTRL_TOG	0x00C

#define HW_RTC_CTRL_SFTRST			__BIT(31)
#define HW_RTC_CTRL_CLKGATE			__BIT(30)
#define HW_RTC_CTRL_RSVD0			__BITS(29, 7)
#define HW_RTC_CTRL_SUPPRESS_COPY2ANALOG	__BIT(6)
#define HW_RTC_CTRL_FORCE_UPDATE		__BIT(5)
#define HW_RTC_CTRL_WATCHDOGEN			__BIT(4)
#define HW_RTC_CTRL_ONEMSEC_IRQ			__BIT(3)
#define HW_RTC_CTRL_ALARM_IRQ			__BIT(2)
#define HW_RTC_CTRL_ONEMSEC_IRQ_EN		__BIT(1)
#define HW_RTC_CTRL_ALARM_IRQ_EN		__BIT(0)

/*
 * Real-Time Clock Status Register.
 */
#define HW_RTC_STAT	0x010
#define HW_RTC_STAT_SET	0x014
#define HW_RTC_STAT_CLR	0x018
#define HW_RTC_STAT_TOG	0x01C

#define HW_RTC_STAT_RTC_PRESENT		__BIT(31)
#define HW_RTC_STAT_ALARM_PRESENT	__BIT(30)
#define HW_RTC_STAT_WATCHDOG_PRESENT	__BIT(29)
#define HW_RTC_STAT_XTAL32000_PRESENT	__BIT(28)
#define HW_RTC_STAT_XTAL32768_PRESENT	__BIT(27)
#define HW_RTC_STAT_RSVD1		__BITS(26, 24)
#define HW_RTC_STAT_STALE_REGS		__BIT(23, 16)
#define HW_RTC_STAT_NEW_REGS		__BIT(15, 8)
#define HW_RTC_STAT_RSVD0		__BIT(7, 0)

/*
 * Real-Time Clock Milliseconds Counter.
 */
#define HW_RTC_MILLISECONDS	0x020
#define HW_RTC_MILLISECONDS_SET	0x024
#define HW_RTC_MILLISECONDS_CLR	0x028
#define HW_RTC_MILLISECONDS_TOG	0x02C

#define HW_RTC_MILLISECONDS_COUNT	__BITS(31, 0)

/*
 * Real-Time Clock Seconds Counter.
 */
#define HW_RTC_SECONDS		0x030
#define HW_RTC_SECONDS_SET	0x034
#define HW_RTC_SECONDS_CLR	0x038
#define HW_RTC_SECONDS_TOG	0x03C

#define HW_RTC_SECONDS_COUNT	__BITS(31, 0)

/*
 * Real-Time Clock Alarm Register.
 */
#define HW_RTC_ALARM		0x040
#define HW_RTC_ALARM_SET	0x044
#define HW_RTC_ALARM_CLR	0x048
#define HW_RTC_ALARM_TOG	0x04C

#define HW_RTC_ALARM_VALUE	__BITS(31, 0)

/*
 * Watchdog Timer Register.
 */
#define HW_RTC_WATCHDOG		0x050
#define HW_RTC_WATCHDOG_SET	0x054
#define HW_RTC_WATCHDOG_CLR	0x058
#define HW_RTC_WATCHDOG_TOG	0x05C

#define HW_RTC_WATCHDOG_COUNT	__BITS(31, 0)

/*
 * Persistent State Register 0.
 */
#define HW_RTC_PERSISTENT0	0x060
#define HW_RTC_PERSISTENT0_SET	0x064
#define HW_RTC_PERSISTENT0_CLR	0x068
#define HW_RTC_PERSISTENT0_TOG	0x06C

#define HW_RTC_PERSISTENT0_SPARE_ANALOG		__BITS(31, 18)
#define HW_RTC_PERSISTENT0_AUTO_RESTART		__BIT(17)
#define HW_RTC_PERSISTENT0_DISABLE_PSWITCH	__BIT(16)
#define HW_RTC_PERSISTENT0_LOWERBIAS		__BITS(15, 14)
#define HW_RTC_PERSISTENT0_DISABLE_XTALOK	__BIT(13)
#define HW_RTC_PERSISTENT0_MSEC_RES		__BITS(12, 8)
#define HW_RTC_PERSISTENT0_ALARM_WAKE		__BIT(7)
#define HW_RTC_PERSISTENT0_XTAL32_FREQ		__BIT(6)
#define HW_RTC_PERSISTENT0_XTAL32KHZ_PWRUP	__BIT(5)
#define HW_RTC_PERSISTENT0_XTAL24MHZ_PWRUP	__BIT(4)
#define HW_RTC_PERSISTENT0_LCK_SECS		__BIT(3)
#define HW_RTC_PERSISTENT0_ALARM_EN		__BIT(2)
#define HW_RTC_PERSISTENT0_ALARM_WAKE_EN	__BIT(1)
#define HW_RTC_PERSISTENT0_CLOCKSOURCE		__BIT(0)

/*
 * Persistent State Register 1.
 */
#define HW_RTC_PERSISTENT1	0x070
#define HW_RTC_PERSISTENT1_SET	0x074
#define HW_RTC_PERSISTENT1_CLR	0x078
#define HW_RTC_PERSISTENT1_TOG	0x07C

#define HW_RTC_PERSISTENT1_GENERAL	__BITS(31, 0)

/*
 * Persistent State Register 2.
 */
#define HW_RTC_PERSISTENT2	0x080
#define HW_RTC_PERSISTENT2_SET	0x084
#define HW_RTC_PERSISTENT2_CLR	0x088
#define HW_RTC_PERSISTENT2_TOG	0x08C

#define HW_RTC_PERSISTENT2_GENERAL	__BITS(31, 0)

/*
 * Persistent State Register 3.
 */
#define HW_RTC_PERSISTENT3	0x090
#define HW_RTC_PERSISTENT3_SET	0x094
#define HW_RTC_PERSISTENT3_CLR	0x098
#define HW_RTC_PERSISTENT3_TOG	0x09C

#define HW_RTC_PERSISTENT3_GENERAL	__BITS(31, 0)

/*
 * Persistent State Register 4.
 */
#define HW_RTC_PERSISTENT4	0x0A0
#define HW_RTC_PERSISTENT4_SET	0x0A4
#define HW_RTC_PERSISTENT4_CLR	0x0A8
#define HW_RTC_PERSISTENT4_TOG	0x0AC

#define HW_RTC_PERSISTENT4_GENERAL	__BITS(31, 0)

/*
 * Persistent State Register 5.
 */
#define HW_RTC_PERSISTENT5	0x0B0
#define HW_RTC_PERSISTENT5_SET	0x0B4
#define HW_RTC_PERSISTENT5_CLR	0x0B8
#define HW_RTC_PERSISTENT5_TOG	0x0BC

#define HW_RTC_PERSISTENT5_GENERAL	__BITS(31, 0)

/*
 * Real-Time Clock Debug Register.
 */
#define HW_RTC_DEBUG		0x0C0
#define HW_RTC_DEBUG_SET	0x0C4
#define HW_RTC_DEBUG_CLR	0x0C8
#define HW_RTC_DEBUG_TOG	0x0CC

#define HW_RTC_DEBUG_RSVD0			__BITS(31, 2)
#define HW_RTC_DEBUG_WATCHDOG_RESET_MASK	__BIT(1)
#define HW_RTC_DEBUG_WATCHDOG_RESET		__BIT(0)

/*
 * Real-Time Clock Version Register.
 */
#define HW_RTC_VERSION	0x0D0

#define HW_RTC_VERSION_MAJOR	__BITS(31, 24)
#define HW_RTC_VERSION_MINOR	__BITS(23, 16)
#define HW_RTC_VERSION_STEP	__BITS(15, 9)

#endif /* !_ARM_IMX_IMX23_RTCREG_H_ */
