/* $NetBSD: soc_tegra124.c,v 1.17.14.2 2020/04/08 14:07:30 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: soc_tegra124.c,v 1.17.14.2 2020/04/08 14:07:30 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <arm/cpufunc.h>

#include <evbarm/fdt/machdep.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_var.h>

#define EVP_RESET_VECTOR_0_REG	0x100

int
tegra124_mpstart(void)
{
	int ret = 0;

#if defined(MULTIPROCESSOR)
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_subregion(bst, tegra_ppsb_bsh,
	    TEGRA_EVP_OFFSET, TEGRA_EVP_SIZE, &bsh);

	arm_cpu_max = 1 + __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU);
	KASSERT(arm_cpu_max == 4);

	bus_space_write_4(bst, bsh, EVP_RESET_VECTOR_0_REG,
	    (uint32_t)KERN_VTOPHYS((vaddr_t)cpu_mpstart));
	bus_space_barrier(bst, bsh, EVP_RESET_VECTOR_0_REG, 4,
	    BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE);

	for (u_int cpuindex = 1; cpuindex < arm_cpu_max; cpuindex++) {
		static u_int tegra_cpu_pmu[] = {
		    0,
		    PMC_PARTID_CPU1,
		    PMC_PARTID_CPU2,
		    PMC_PARTID_CPU3
		};

		tegra_pmc_power(tegra_cpu_pmu[cpuindex], true);

		u_int i;
		for (i = 0x10000000; i > 0; i--) {
			if (cpu_hatched_p(cpuindex))
				break;
		}

		if (i == 0) {
			ret++;
			aprint_error("cpu%d: WARNING: AP failed to start\n",
			    cpuindex);
		}
	}
#endif
	return ret;
}
