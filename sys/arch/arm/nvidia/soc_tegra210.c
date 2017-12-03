/* $NetBSD: soc_tegra210.c,v 1.1.10.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: soc_tegra210.c,v 1.1.10.2 2017/12/03 11:35:54 jdolecek Exp $");

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

#define EVP_RESET_VECTOR_0_REG	0x100

void
tegra210_mpinit(void)
{
#if defined(MULTIPROCESSOR)
	extern void cortex_mpstart(void);
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_subregion(bst, tegra_ppsb_bsh,
	    TEGRA_EVP_OFFSET, TEGRA_EVP_SIZE, &bsh);

	arm_cpu_max = 4;

	bus_space_write_4(bst, bsh, EVP_RESET_VECTOR_0_REG,
	    (uint32_t)cortex_mpstart);
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
