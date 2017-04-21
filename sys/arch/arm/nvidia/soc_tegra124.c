/* $NetBSD: soc_tegra124.c,v 1.12.4.1 2017/04/21 16:53:23 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: soc_tegra124.c,v 1.12.4.1 2017/04/21 16:53:23 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/clk/clk.h>
#include <dev/i2c/i2cvar.h>
#include <dev/fdt/fdtvar.h>

#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#define EVP_RESET_VECTOR_0_REG	0x100

#define FUSE_SKU_INFO_REG	0x010
#define FUSE_CPU_SPEEDO_0_REG	0x014
#define FUSE_CPU_IDDQ_REG	0x018
#define FUSE_FT_REV_REG		0x028
#define FUSE_CPU_SPEEDO_1_REG	0x02c
#define FUSE_CPU_SPEEDO_2_REG	0x030
#define FUSE_SOC_SPEEDO_0_REG	0x034
#define FUSE_SOC_SPEEDO_1_REG	0x038
#define FUSE_SOC_SPEEDO_2_REG	0x03c
#define FUSE_SOC_IDDQ_REG	0x040
#define FUSE_GPU_IDDQ_REG	0x128

static void	tegra124_speedo_init(void);
static int	tegra124_speedo_init_ids(uint32_t);
static bool	tegra124_speedo_rate_ok(u_int);

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
	{ 2316, 1, 193, 0 },
	{ 2100, 1, 175, 0 },
	{ 1896, 1, 158, 0 },
	{ 1692, 1, 141, 0 },
	{ 1500, 1, 125, 0 },
	{ 1296, 1, 108, 0 },
	{ 1092, 1, 91, 0 },
	{ 900, 1, 75, 0 },
	{ 696, 1, 58, 0 }
};

static const u_int tegra124_cpufreq_max[] = {
	2014,
	2320,
	2116,
	2524
};

static struct tegra124_speedo {
	u_int cpu_speedo_id;
	u_int soc_speedo_id;
	u_int gpu_speedo_id;
} tegra124_speedo = {
	.cpu_speedo_id = 0,
	.soc_speedo_id = 0,
	.gpu_speedo_id = 0
};

static struct clk *tegra124_clk_pllx = NULL;

void
tegra124_cpuinit(void)
{
	int i2c_node = OF_finddevice("/i2c@7000d000");
	if (i2c_node == -1)
		i2c_node = OF_finddevice("/i2c@0,7000d000"); /* old DTB */
	if (i2c_node == -1) {
		aprint_error("cpufreq: ERROR: couldn't find i2c@7000d000\n");
		return;
	}
	i2c_tag_t ic = fdtbus_get_i2c_tag(i2c_node);

	/* Set VDD_CPU voltage to 1.4V */
	const u_int target_mv = 1400;
	const u_int sd0_vsel = (target_mv - 600) / 10;
	uint8_t data[2] = { 0x00, sd0_vsel };

	iic_acquire_bus(ic, I2C_F_POLL);
	const int error = iic_exec(ic, I2C_OP_WRITE_WITH_STOP, 0x40,
	    NULL, 0, data, sizeof(data), I2C_F_POLL);
	iic_release_bus(ic, I2C_F_POLL);
	if (error) {
		aprint_error("cpufreq: ERROR: couldn't set VDD_CPU: %d\n",
		    error);
		return;
	}
	delay(10000);

	tegra124_speedo_init();

	int cpu_node = OF_finddevice("/cpus/cpu@0");
	if (cpu_node != -1)
		tegra124_clk_pllx = fdtbus_clock_get(cpu_node, "pll_x");
	if (tegra124_clk_pllx == NULL) {
		aprint_error("cpufreq: ERROR: couldn't find pll_x\n");
		return;
	}

	tegra_cpufreq_register(&tegra124_cpufreq_func);
}

static void
tegra124_speedo_init(void)
{
	uint32_t sku_id;

	sku_id = tegra_fuse_read(FUSE_SKU_INFO_REG);
	tegra124_speedo_init_ids(sku_id);
}

static int
tegra124_speedo_init_ids(uint32_t sku_id)
{
	int threshold = 0;

	switch (sku_id) {
	case 0x00:
	case 0x0f:
	case 0x23:
		break;	/* use default */
	case 0x83:
		tegra124_speedo.cpu_speedo_id = 2;
		break;
	case 0x1f:
	case 0x87:
	case 0x27:
		tegra124_speedo.cpu_speedo_id = 2;
		tegra124_speedo.soc_speedo_id = 0;
		tegra124_speedo.gpu_speedo_id = 1;
		break;
	case 0x81:
	case 0x21:
	case 0x07:
		tegra124_speedo.cpu_speedo_id = 1;
		tegra124_speedo.soc_speedo_id = 1;
		tegra124_speedo.gpu_speedo_id = 1;
		threshold = 1;
		break;
	case 0x49:
	case 0x4a:
	case 0x48:
		tegra124_speedo.cpu_speedo_id = 4;
		tegra124_speedo.soc_speedo_id = 2;
		tegra124_speedo.gpu_speedo_id = 3;
		threshold = 1;
		break;
	default:
		aprint_error("tegra124: unknown SKU ID %#x\n", sku_id);
		break;	/* use default */
	}

	return threshold;
}

static bool
tegra124_speedo_rate_ok(u_int rate)
{
	u_int tbl = 0;

	if (tegra124_speedo.cpu_speedo_id < __arraycount(tegra124_cpufreq_max))
		tbl = tegra124_speedo.cpu_speedo_id;

	return rate <= tegra124_cpufreq_max[tbl];
}


static u_int
tegra124_cpufreq_set_rate(u_int rate)
{
	const u_int nrates = __arraycount(tegra124_cpufreq_rates);
	const struct tegra124_cpufreq_rate *r = NULL;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int error;

	if (tegra124_speedo_rate_ok(rate) == false)
		return EINVAL;

	for (int i = 0; i < nrates; i++) {
		if (tegra124_cpufreq_rates[i].rate == rate) {
			r = &tegra124_cpufreq_rates[i];
			break;
		}
	}
	if (r == NULL)
		return EINVAL;

	error = clk_set_rate(tegra124_clk_pllx, r->rate * 1000000);
	if (error == 0) {
		rate = tegra124_cpufreq_get_rate();
		for (CPU_INFO_FOREACH(cii, ci)) {
			ci->ci_data.cpu_cc_freq = rate * 1000000;
		}
	}

	return error;
}

static u_int
tegra124_cpufreq_get_rate(void)
{
	return clk_get_rate(tegra124_clk_pllx) / 1000000;
}

static size_t
tegra124_cpufreq_get_available(u_int *pavail, size_t maxavail)
{
	const u_int nrates = __arraycount(tegra124_cpufreq_rates);
	u_int n, cnt;

	KASSERT(nrates <= maxavail);

	for (n = 0, cnt = 0; n < nrates; n++) {
		if (tegra124_speedo_rate_ok(tegra124_cpufreq_rates[n].rate)) {
			pavail[cnt++] = tegra124_cpufreq_rates[n].rate;
		}
	}

	return cnt;
}

void
tegra124_mpinit(void)
{
#if defined(MULTIPROCESSOR)
	extern void cortex_mpstart(void);
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_subregion(bst, tegra_ppsb_bsh,
	    TEGRA_EVP_OFFSET, TEGRA_EVP_SIZE, &bsh);

	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
	KASSERT(arm_cpu_max == 4);

	bus_space_write_4(bst, bsh, EVP_RESET_VECTOR_0_REG, (uint32_t)cortex_mpstart);
	bus_space_barrier(bst, bsh, EVP_RESET_VECTOR_0_REG, 4,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);
	uint32_t started = 0;

	tegra_pmc_power(PMC_PARTID_CPU1, true); started |= __BIT(1);
	tegra_pmc_power(PMC_PARTID_CPU2, true); started |= __BIT(2);
	tegra_pmc_power(PMC_PARTID_CPU3, true); started |= __BIT(3);

	for (u_int i = 0x10000000; i > 0; i--) {
		arm_dmb();
		if (arm_cpu_hatched == started)
			break;
	}
#endif
}
