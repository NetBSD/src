/* $NetBSD: tegra_timerreg.h,v 1.2.16.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#ifndef _ARM_TEGRA_TIMERREG_H
#define _ARM_TEGRA_TIMERREG_H

#define TMR1_PTV_REG		0x00
#define TMR1_PCR_REG		0x04
#define TMR2_PTV_REG		0x08
#define TMR2_PCR_REG		0x0c
#define TMRUS_CNTR_1US_REG	0x10
#define TMRUS_USEC_CFG_REG	0x14
#define TMRUS_CNTR_FREEZE_REG	0x18
#define TMR3_PTV_REG		0x50
#define TMR3_PCR_REG		0x54
#define TMR4_PTV_REG		0x58
#define TMR4_PCR_REG		0x5c
#define TMR5_PTV_REG		0x60
#define TMR5_PCR_REG		0x64
#define TMR6_PTV_REG		0x68
#define TMR6_PCR_REG		0x6c
#define TMR7_PTV_REG		0x70
#define TMR7_PCR_REG		0x74
#define TMR8_PTV_REG		0x78
#define TMR8_PCR_REG		0x7c
#define TMR9_PTV_REG		0x80
#define TMR9_PCR_REG		0x84
#define TMR0_PTV_REG		0x88
#define TMR0_PCR_REG		0x8c

#define TMR_PTV_EN		__BIT(31)
#define TMR_PTV_PER		__BIT(30)
#define TMR_PTV_VAL		__BITS(28,0)

#define TMR_PCR_INTR_CLR	__BIT(30)
#define TMR_PCR_VAL		__BITS(28,0)

#define WDT0_CONFIG_REG		0x100
#define WDT0_STATUS_REG		0x104
#define WDT0_COMMAND_REG	0x108
#define WDT0_UNLOCK_PATTERN_REG	0x10c
#define WDT1_CONFIG_REG		0x100
#define WDT1_STATUS_REG		0x104
#define WDT1_COMMAND_REG	0x108
#define WDT1_UNLOCK_PATTERN_REG	0x10c
#define WDT2_CONFIG_REG		0x100
#define WDT2_STATUS_REG		0x104
#define WDT2_COMMAND_REG	0x108
#define WDT2_UNLOCK_PATTERN_REG	0x10c
#define WDT3_CONFIG_REG		0x100
#define WDT3_STATUS_REG		0x104
#define WDT3_COMMAND_REG	0x108
#define WDT3_UNLOCK_PATTERN_REG	0x10c
#define WDT4_CONFIG_REG		0x100
#define WDT4_STATUS_REG		0x104
#define WDT4_COMMAND_REG	0x108
#define WDT4_UNLOCK_PATTERN_REG	0x10c

#endif /* _ARM_TEGRA_TIMERREG_H */
