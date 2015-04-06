/* $NetBSD: tegra_intr.h,v 1.2.2.2 2015/04/06 15:17:53 skrll Exp $ */

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

#ifndef _ARM_TEGRA_INTR_H
#define _ARM_TEGRA_INTR_H

#define PIC_MAXSOURCES		256
#define PIC_MAXMAXSOURCES	(PIC_MAXSOURCES + 32)

#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gtmr_intr.h>

#define TEGRA_INTR(x)		((x) + 32)

#define TEGRA_INTR_SDMMC1	TEGRA_INTR(14)
#define TEGRA_INTR_SDMMC2	TEGRA_INTR(15)
#define TEGRA_INTR_SDMMC3	TEGRA_INTR(19)
#define TEGRA_INTR_USB1		TEGRA_INTR(20)
#define TEGRA_INTR_USB2		TEGRA_INTR(21)
#define TEGRA_INTR_SATA		TEGRA_INTR(23)
#define TEGRA_INTR_SDMMC4	TEGRA_INTR(31)
#define TEGRA_INTR_UARTA	TEGRA_INTR(36)
#define TEGRA_INTR_UARTB	TEGRA_INTR(37)
#define TEGRA_INTR_UARTC	TEGRA_INTR(46)
#define TEGRA_INTR_HDA		TEGRA_INTR(81)
#define TEGRA_INTR_UARTD	TEGRA_INTR(90)
#define TEGRA_INTR_USB3		TEGRA_INTR(97)

#endif /* _ARM_TEGRA_INTR_H */
