/*	$NetBSD: am335x_prcm.c,v 1.1.6.2 2013/02/25 00:28:31 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: am335x_prcm.c,v 1.1.6.2 2013/02/25 00:28:31 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_prcm.h>

#define AM335X_CLKCTRL_MODULEMODE_MASK		__BITS(0, 1)
#define   AM335X_CLKCTRL_MODULEMODE_DISABLED	0
#define   AM335X_CLKCTRL_MODULEMODE_ENABLE	2

static void
am335x_prcm_check_clkctrl(bus_size_t cm_module,
    bus_size_t clkctrl_reg, uint32_t v)
{
#ifdef DIAGNOSTIC
	uint32_t u = prcm_read_4(cm_module, clkctrl_reg);

	if (__SHIFTOUT(u, AM335X_CLKCTRL_MODULEMODE_MASK) !=
	    __SHIFTOUT(v, AM335X_CLKCTRL_MODULEMODE_MASK))
		aprint_error("clkctrl didn't take: %"PRIx32" -/-> %"PRIx32"\n",
		    u, v);
#else
	(void)cm_module;
	(void)clkctrl_reg;
	(void)v;
#endif
}

void
prcm_module_enable(const struct omap_module *om)
{
	bus_size_t cm_module = om->om_prcm_cm_module;
	bus_size_t clkctrl_reg = om->om_prcm_cm_clkctrl_reg;
	uint32_t clkctrl;

	clkctrl = prcm_read_4(cm_module, clkctrl_reg);
	clkctrl &=~ AM335X_CLKCTRL_MODULEMODE_MASK;
	clkctrl |= __SHIFTIN(AM335X_CLKCTRL_MODULEMODE_ENABLE,
	    AM335X_CLKCTRL_MODULEMODE_MASK);
	prcm_write_4(cm_module, clkctrl_reg, clkctrl);
	am335x_prcm_check_clkctrl(cm_module, clkctrl_reg, clkctrl);
}

void
prcm_module_disable(const struct omap_module *om)
{
	bus_size_t cm_module = om->om_prcm_cm_module;
	bus_size_t clkctrl_reg = om->om_prcm_cm_clkctrl_reg;
	uint32_t clkctrl;

	clkctrl = prcm_read_4(cm_module, clkctrl_reg);
	clkctrl &=~ AM335X_CLKCTRL_MODULEMODE_MASK;
	clkctrl |= __SHIFTIN(AM335X_CLKCTRL_MODULEMODE_DISABLED,
	    AM335X_CLKCTRL_MODULEMODE_MASK);
	prcm_write_4(cm_module, clkctrl_reg, clkctrl);
	am335x_prcm_check_clkctrl(cm_module, clkctrl_reg, clkctrl);
}
