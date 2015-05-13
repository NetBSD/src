/* $NetBSD: soc_tegra124.c,v 1.3 2015/05/13 11:06:13 jmcneill Exp $ */

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

#include "opt_tegra.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: soc_tegra124.c,v 1.3 2015/05/13 11:06:13 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#define EVP_RESET_VECTOR_0_REG	0x100

static u_int	tegra124_cpufreq_set_rate(u_int);
static u_int	tegra124_cpufreq_get_rate(void);
static size_t	tegra124_cpufreq_get_available(u_int *, size_t);

static const struct tegra_cpufreq_func tegra124_cpufreq_func = {
	.set_rate = tegra124_cpufreq_set_rate,
	.get_rate = tegra124_cpufreq_get_rate,
	.get_available = tegra124_cpufreq_get_available,
};

static struct tegra124_cpufreq_rate {
	u_int rate;
	u_int divm;
	u_int divn;
	u_int divp;
} tegra124_cpufreq_rates[] = {
	{ 2292, 1, 191, 0 },
	{ 2100, 1, 175, 0 },
	{ 1896, 1, 158, 0 },
	{ 1692, 1, 141, 0 },
	{ 1500, 1, 125, 0 },
	{ 1296, 1, 108, 0 },
	{ 1092, 1, 91, 0 },
	{ 900, 1, 75, 0 },
	{ 696, 1, 58, 0 }
};

void
tegra124_cpuinit(void)
{
	tegra_cpufreq_register(&tegra124_cpufreq_func);
}

static u_int
tegra124_cpufreq_set_rate(u_int rate)
{
	const u_int nrates = __arraycount(tegra124_cpufreq_rates);
	const struct tegra124_cpufreq_rate *r = NULL;

	for (int i = 0; i < nrates; i++) {
		if (tegra124_cpufreq_rates[i].rate == rate) {
			r = &tegra124_cpufreq_rates[i];
			break;
		}
	}
	if (r == NULL)
		return EINVAL;

	tegra_car_pllx_set_rate(r->divm, r->divn, r->divp);

	return 0;
}

static u_int
tegra124_cpufreq_get_rate(void)
{
	return tegra_car_pllx_rate() / 1000000;
}

static size_t
tegra124_cpufreq_get_available(u_int *pavail, size_t maxavail)
{
	const u_int nrates = __arraycount(tegra124_cpufreq_rates);
	u_int n;

	KASSERT(nrates <= maxavail);

	for (n = 0; n < nrates; n++) {
		pavail[n] = tegra124_cpufreq_rates[n].rate;
	}

	return nrates;
}

void
tegra124_mpinit(void)
{
#if defined(MULTIPROCESSOR)
	extern void cortex_mpstart(void);
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	u_int i;

	bus_space_subregion(bst, tegra_ppsb_bsh,
	    TEGRA_EVP_OFFSET, TEGRA_EVP_SIZE, &bsh);

	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
	KASSERT(arm_cpu_max == 4);

	bus_space_write_4(bst, bsh, EVP_RESET_VECTOR_0_REG, (uint32_t)cortex_mpstart);
	bus_space_barrier(bst, bsh, EVP_RESET_VECTOR_0_REG, 4,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	tegra_pmc_power(PMC_PARTID_CPU1, true);
	tegra_pmc_power(PMC_PARTID_CPU2, true);
	tegra_pmc_power(PMC_PARTID_CPU3, true);

	for (i = 0x10000000; i > 0; i--) {
		__asm __volatile("dmb" ::: "memory");
		if (arm_cpu_hatched == 0xe)
			break;
	}
#endif
}
