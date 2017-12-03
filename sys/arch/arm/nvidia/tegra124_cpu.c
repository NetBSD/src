/* $NetBSD: tegra124_cpu.c,v 1.4.10.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra124_cpu.c,v 1.4.10.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>


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

static int	tegra124_cpu_match(device_t, cfdata_t, void *);
static void	tegra124_cpu_attach(device_t, device_t, void *);
static int	tegra124_cpu_init_cpufreq(device_t);

CFATTACH_DECL_NEW(tegra124_cpu, 0, tegra124_cpu_match, tegra124_cpu_attach,
    NULL, NULL);

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
	u_int uvol;
} tegra124_cpufreq_rates[] = {
	{ 2316, 1, 193, 0, 1360000 },
	{ 2100, 1, 175, 0, 1260000 },
	{ 1896, 1, 158, 0, 1180000 },
	{ 1692, 1, 141, 0, 1100000 },
	{ 1500, 1, 125, 0, 1020000 },
	{ 1296, 1, 108, 0, 960000 },
	{ 1092, 1, 91,  0, 900000 },
	{ 900,  1, 75,  0, 840000 },
	{ 696,  1, 58,  0, 800000 }
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
static struct fdtbus_regulator *tegra124_reg_vddcpu = NULL;

static int
tegra124_cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124", NULL };
	struct fdt_attach_args *faa = aux;

	if (OF_finddevice("/cpus/cpu@0") != faa->faa_phandle)
		return 0;

	return of_match_compatible(OF_finddevice("/"), compatible);
}

static void
tegra124_cpu_attach(device_t parent, device_t self, void *aux)
{
	aprint_naive("\n");
	aprint_normal(": DVFS\n");

	config_finalize_register(self, tegra124_cpu_init_cpufreq);
}

static int
tegra124_cpu_init_cpufreq(device_t dev)
{
	tegra124_speedo_init();

	int cpu_node = OF_finddevice("/cpus/cpu@0");
	if (cpu_node != -1) {
		tegra124_clk_pllx = fdtbus_clock_get(cpu_node, "pll_x");
		tegra124_reg_vddcpu = fdtbus_regulator_acquire(cpu_node,
		    "vdd-cpu-supply");
	}
	if (tegra124_clk_pllx == NULL) {
		aprint_error_dev(dev, "couldn't find clock pll_x\n");
		return 0;
	}
	if (tegra124_reg_vddcpu == NULL) {
		aprint_error_dev(dev, "couldn't find voltage regulator\n");
		return 0;
	}

	tegra_cpufreq_register(&tegra124_cpufreq_func);

	return 0;
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
	u_int cur_uvol;
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

	error = fdtbus_regulator_get_voltage(tegra124_reg_vddcpu, &cur_uvol);
	if (error != 0)
		return error;

	if (cur_uvol < r->uvol) {
		error = fdtbus_regulator_set_voltage(tegra124_reg_vddcpu,
		    r->uvol, r->uvol);
		if (error != 0)
			return error;
	}

	error = clk_set_rate(tegra124_clk_pllx, r->rate * 1000000);
	if (error == 0) {
		rate = tegra124_cpufreq_get_rate();
		for (CPU_INFO_FOREACH(cii, ci)) {
			ci->ci_data.cpu_cc_freq = rate * 1000000;
		}
	}

	if (cur_uvol > r->uvol) {
		(void)fdtbus_regulator_set_voltage(tegra124_reg_vddcpu,
		    r->uvol, r->uvol);
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
