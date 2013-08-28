/*	$NetBSD: am335x_prcm.c,v 1.2.2.1 2013/08/28 23:59:12 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: am335x_prcm.c,v 1.2.2.1 2013/08/28 23:59:12 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/pmf.h>

#include <arm/omap/am335x_prcm.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_prcm.h>
#include <arm/omap/omap_var.h>

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

static const uint16_t mpu_frequencies[] = {
	550,
	600,
	650,
	700,
	720,
	800,
	900,
	1000
};
static int mpu_frequency;

void
prcm_mpu_pll_config(u_int mpupll_m)
{
	uint32_t clkmode = prcm_read_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_CLKMODE_DPLL_MPU);
	uint32_t clksel = prcm_read_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_CLKSEL_DPLL_MPU);
	//uint32_t div_m2 = prcm_read_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_DIV_M2_DPLL_MPU);

	/* Request the DPLL be put into bypass mode */
	prcm_write_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_CLKMODE_DPLL_MPU, AM335X_PRCM_CM_CLKMODE_DPLL_MN_BYP_MODE);

	/* Wait for it to go into bypass */
	while (prcm_read_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_IDLEST_DPLL_MPU) != AM335X_PRCM_CM_IDLEST_DPLL_ST_DPLL_CLK_MN_BYPASS) {
		/* nothing */
	}

	/* Replace multipler */
	clksel &= ~AM335X_PRCM_CM_CLKSEL_DPLL_MULT;
	clksel |= __SHIFTIN(mpupll_m, AM335X_PRCM_CM_CLKSEL_DPLL_MULT);
	prcm_write_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_CLKSEL_DPLL_MPU, clksel);

	/* Exit bypass mode */
	clkmode |= AM335X_PRCM_CM_CLKMODE_DPLL_LOCK_MODE; 
	prcm_write_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_CLKMODE_DPLL_MPU, clkmode);

	/* Wait for the DPLL to lock */
	while (prcm_read_4(AM335X_PRCM_CM_WKUP, AM335X_PRCM_CM_IDLEST_DPLL_MPU) != AM335X_PRCM_CM_IDLEST_DPLL_ST_DPLL_CLK_LOCKED) {
		/* nothing */
	}
}

static int
mpu_current_frequency_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int freq = mpu_frequency;
	int old_freq = freq;

	KASSERTMSG(curcpu()->ci_data.cpu_cc_freq == mpu_frequency * 1000000,
	    "cc_freq %"PRIu64" mpu_freq %u000000",
	    curcpu()->ci_data.cpu_cc_freq, mpu_frequency);

	node.sysctl_data = &freq;

	int error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	KASSERT(old_freq == mpu_frequency);

	error = EINVAL;
	for (size_t i = 0; i < __arraycount(mpu_frequencies); i++) {
		if (mpu_frequencies[i] == freq) {
			error = 0;
			break;
		}
	}
	if (error)
		return EINVAL;

	if (freq != old_freq) {
		const int s = splhigh();
		prcm_mpu_pll_config(freq);
		curcpu()->ci_data.cpu_cc_freq = freq * 1000000;
		mpu_frequency = freq;
		splx(s);
		aprint_normal_dev(curcpu()->ci_dev,
		    "frequency changed from %d MHz to %d MHz\n",
		    old_freq, freq);
		pmf_event_inject(NULL, PMFE_SPEED_CHANGED);
	}

	return 0;
}

SYSCTL_SETUP(sysctl_am335x_machdep_setup, "sysctl am335x machdep subtree setup")
{
	const struct sysctlnode *freqnode, *node;

	static char mpu_available_frequencies[__arraycount(mpu_frequencies)*6];

	__CTASSERT(__arraycount(mpu_frequencies) == 8);
	snprintf(mpu_available_frequencies, sizeof(mpu_available_frequencies),
	   "%u %u %u %u %u %u %u %u",
	   mpu_frequencies[0], mpu_frequencies[1], mpu_frequencies[2],
	   mpu_frequencies[3], mpu_frequencies[4], mpu_frequencies[5],
	   mpu_frequencies[6], mpu_frequencies[7]);
	mpu_frequency = curcpu()->ci_data.cpu_cc_freq / 1000000;

	sysctl_createv(clog, 0, NULL, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, &node, &freqnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "frequency", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "available", NULL,
		       NULL, 0, mpu_available_frequencies,
		       strlen(mpu_available_frequencies),
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &freqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "current", NULL,
		       mpu_current_frequency_sysctl_helper, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
}

void
am335x_sys_clk(bus_space_handle_t ctlmode_ioh)
{
	static const uint32_t sys_clks[4] = {
		[0] = 19200000, [1] = 24000000, [2] = 25000000, [3] = 26000000
	};

	const uint32_t control_status = bus_space_read_4(&omap_bs_tag,
		ctlmode_ioh, CTLMOD_CONTROL_STATUS);
	omap_sys_clk = sys_clks[__SHIFTOUT(control_status, CTLMOD_CONTROL_STATUS_SYSBOOT1)];
}

void
am335x_cpu_clk(void)
{
	const bus_size_t cm_wkup = AM335X_PRCM_CM_WKUP;
	const uint32_t clksel = prcm_read_4(cm_wkup, AM335X_PRCM_CM_CLKSEL_DPLL_MPU);
	const uint32_t div_m2 = prcm_read_4(cm_wkup, AM335X_PRCM_CM_DIV_M2_DPLL_MPU);
	const uint32_t m = __SHIFTOUT(clksel, AM335X_PRCM_CM_CLKSEL_DPLL_MULT);
	const uint32_t n = __SHIFTOUT(clksel, AM335X_PRCM_CM_CLKSEL_DPLL_DIV);
	const uint32_t m2 = __SHIFTOUT(div_m2, AM335X_PRCM_CM_DIV_M2_DPLL_CLKOUT_DIV);
	/* XXX This ignores CM_CLKSEL_DPLL_MPU[DPLL_REGM4XEN].  */
	curcpu()->ci_data.cpu_cc_freq = (m * ((uint64_t)omap_sys_clk / (n + 1))) / m2;
	printf("%s: %"PRIu64": sys_clk=%u m=%u n=%u (%u) m2=%u\n",
	    __func__, curcpu()->ci_data.cpu_cc_freq,
	    omap_sys_clk, m, n, n+1, m2);
}
