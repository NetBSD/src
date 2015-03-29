/* $NetBSD: tegra_soc.c,v 1.2 2015/03/29 22:27:04 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_soc.c,v 1.2 2015/03/29 22:27:04 jmcneill Exp $");

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

bus_space_handle_t tegra_host1x_bsh;
bus_space_handle_t tegra_apb_bsh;
bus_space_handle_t tegra_ahb_a2_bsh;

struct arm32_bus_dma_tag tegra_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

#if defined(MULTIPROCESSOR)
static void	tegra_mpinit(void);
#endif

void
tegra_bootstrap(void)
{
	bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_HOST1X_BASE, TEGRA_HOST1X_SIZE, 0,
	    &tegra_host1x_bsh);
	bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_APB_BASE, TEGRA_APB_SIZE, 0,
	    &tegra_apb_bsh);
	bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_AHB_A2_BASE, TEGRA_AHB_A2_SIZE, 0,
	    &tegra_ahb_a2_bsh);

	curcpu()->ci_data.cpu_cc_freq = 696000000; /* XXX */

#if defined(MULTIPROCESSOR)
	tegra_mpinit();
#endif
}

#if defined(MULTIPROCESSOR)
static void
tegra_mpinit(void)
{
	switch (tegra_chip_id()) {
#ifdef SOC_TEGRA124
	case CHIP_ID_TEGRA124:
		tegra124_mpinit();
		break;
#endif
	default:
		panic("Unsupported SOC ID %#x", tegra_chip_id());
	}
}
#endif

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
