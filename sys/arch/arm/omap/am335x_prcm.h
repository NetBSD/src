/*	$NetBSD: am335x_prcm.h,v 1.1 2012/12/11 18:53:26 riastradh Exp $	*/

/*
 * TI OMAP Power, Reset, and Clock Management on the AM335x
 */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _ARM_OMAP_AM335X_PRCM_H_
#define _ARM_OMAP_AM335X_PRCM_H_

#include <sys/bus.h>

struct omap_module {
	bus_size_t om_prcm_cm_module;
	bus_size_t om_prcm_cm_clkctrl_reg;
};

#define AM335X_PRCM_CM_PER	0x0000
#define AM335X_PRCM_CM_WKUP	0x0400
#define AM335X_PRCM_CM_DPLL	0x0500
#define AM335X_PRCM_CM_MPU	0x0600
#define AM335X_PRCM_CM_DEVICE	0x0700
#define AM335X_PRCM_CM_RTC	0x0800
#define AM335X_PRCM_CM_GFX	0x0900
#define AM335X_PRCM_CM_CEFUSE	0x0a00

#define AM335X_PRCM_PRM_IRQ	0x0b00
#define AM335X_PRCM_PRM_PER	0x0c00
#define AM335X_PRCM_PRM_WKUP	0x0d00
#define AM335X_PRCM_PRM_MPU	0x0e00
#define AM335X_PRCM_PRM_DEVICE	0x0f00
#define AM335X_PRCM_PRM_RTC	0x1000
#define AM335X_PRCM_PRM_GFX	0x1100
#define AM335X_PRCM_PRM_CEFUSE	0x1200

#endif  /* _ARM_OMAP_AM335X_PRCM_H_ */
