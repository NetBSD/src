/* $NetBSD: tegra_rtcreg.h,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_TEGRA_RTCREG_H
#define _ARM_TEGRA_RTCREG_H

#define RTC_CONTROL_REG				0x00
#define RTC_BUSY_REG				0x04
#define RTC_SECONDS_REG				0x08
#define	RTC_SHADOW_SECONDS_REG			0x0c
#define RTC_MILLI_SECONDS_REG			0x10
#define RTC_SECONDS_ALARM0_REG			0x14
#define RTC_SECONDS_ALARM1_REG			0x18
#define RTC_MILLI_SECONDS_ALARM_REG		0x1c
#define RTC_SECONDS_COUNTDOWN_ALARM_REG		0x20
#define RTC_MILLI_SECONDS_COUNTDOW_ALARM_REG	0x24
#define RTC_INTR_MASK_REG			0x28
#define RTC_INTR_STATUS_REG			0x2c
#define RTC_INTR_SOURCE_REG			0x30
#define RTC_INTR_SET_REG			0x34
#define RTC_CORRECTION_FACTOR_REG		0x38

#define RTC_BUSY_STATUS				__BIT(0)

#endif /* _ARM_TEGRA_RTCREG_H */
