/* $NetBSD: tegra_soc.c,v 1.2.2.4 2015/12/27 12:09:31 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_soc.c,v 1.2.2.4 2015/12/27 12:09:31 skrll Exp $");

#define	_ARM32_BUS_DMA_PRIVATE
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_apbreg.h>
#include <arm/nvidia/tegra_mcreg.h>
#include <arm/nvidia/tegra_var.h>

bus_space_handle_t tegra_ppsb_bsh;
bus_space_handle_t tegra_apb_bsh;

struct arm32_bus_dma_tag tegra_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

static void	tegra_mpinit(void);

void
tegra_bootstrap(void)
{
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_PPSB_BASE, TEGRA_PPSB_SIZE, 0,
	    &tegra_ppsb_bsh) != 0)
		panic("couldn't map PPSB");
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_APB_BASE, TEGRA_APB_SIZE, 0,
	    &tegra_apb_bsh) != 0)
		panic("couldn't map APB");

	tegra_mpinit();
}

void
tegra_dma_bootstrap(psize_t psize)
{
}

void
tegra_cpuinit(void)
{
	switch (tegra_chip_id()) {
#ifdef SOC_TEGRA124
	case CHIP_ID_TEGRA124:
		tegra124_cpuinit();
		break;
#endif
	}

	tegra_cpufreq_init();
}

static void
tegra_mpinit(void)
{
#if defined(MULTIPROCESSOR)
	switch (tegra_chip_id()) {
#ifdef SOC_TEGRA124
	case CHIP_ID_TEGRA124:
		tegra124_mpinit();
		break;
#endif
	default:
		panic("Unsupported SOC ID %#x", tegra_chip_id());
	}
#endif
}

u_int
tegra_chip_id(void)
{
	static u_int chip_id = 0;

	if (!chip_id) {
		const bus_space_tag_t bst = &armv7_generic_bs_tag;
		const bus_space_handle_t bsh = tegra_apb_bsh;
		const uint32_t v = bus_space_read_4(bst, bsh,
		    APB_MISC_GP_HIDREV_0_REG);
		chip_id = __SHIFTOUT(v, APB_MISC_GP_HIDREV_0_CHIPID);
	}

	return chip_id;
}

const char *
tegra_chip_name(void)
{
	switch (tegra_chip_id()) {
	case CHIP_ID_TEGRA124:	return "Tegra K1 (T124)";
	case CHIP_ID_TEGRA132:	return "Tegra K1 (T132)";
	default:		return "Unknown Tegra SoC";
	}
}
