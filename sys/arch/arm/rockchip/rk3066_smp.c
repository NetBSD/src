/* $NetBSD: rk3066_smp.c,v 1.2 2021/11/13 15:17:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk3066_smp.c,v 1.2 2021/11/13 15:17:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#define	PMU_PWRDN_CON		0x0008
#define	PMU_PWRDN_ST		0x000c

#define	SRAM_ENTRY_PA		0x0008
#define	SRAM_DOORBELL		0x0004
#define	SRAM_DOORBELL_MAGIC	0xdeadbeaf

extern struct bus_space arm_generic_bs_tag;

static uint32_t
rk3066_mpstart_pa(void)
{
	bool ok __diagused;
	paddr_t pa;

	ok = pmap_extract(pmap_kernel(), (vaddr_t)cpu_mpstart, &pa);
	KASSERT(ok);

	return (uint32_t)pa;
}

static int
rk3066_map(int phandle, bus_space_tag_t bst, bus_space_handle_t *pbsh,
    bus_size_t *psize)
{
	bus_addr_t addr;
	int error;

	error = fdtbus_get_reg(phandle, 0, &addr, psize);
	if (error != 0) {
		return error;
	}

	return bus_space_map(bst, addr, *psize, 0, pbsh);
}

static int
rk3066_smp_enable(int cpus_phandle, u_int cpuno)
{
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh_sram, bsh_pmu;
	bus_size_t sz_sram, sz_pmu;
	uint32_t val;
	int error;

	const int sram_phandle =
	    of_find_bycompat(OF_peer(0), "rockchip,rk3066-smp-sram");
	if (sram_phandle == -1) {
		printf("%s: missing rockchip,rk3066-smp-sram node\n",
		    __func__);
		return ENXIO;
	}

	const int pmu_phandle = fdtbus_get_phandle(cpus_phandle,
	    "rockchip,pmu");
	if (pmu_phandle == -1) {
		printf("%s: missing rockchip,pmu xref\n", __func__);
		return ENXIO;
	}

	error = rk3066_map(sram_phandle, bst, &bsh_sram, &sz_sram);
	if (error != 0) {
		return error;
	}

	error = rk3066_map(pmu_phandle, bst, &bsh_pmu, &sz_pmu);
	if (error != 0) {
		bus_space_unmap(bst, bsh_pmu, sz_pmu);
		return error;
	}

	/* Enable the A17 core's power domain */
	val = bus_space_read_4(bst, bsh_pmu, PMU_PWRDN_CON);
	val &= ~__BIT(cpuno);
	bus_space_write_4(bst, bsh_pmu, PMU_PWRDN_CON, val);

	/* Wait for the A17 core to power on */
	do {
		val = bus_space_read_4(bst, bsh_pmu, PMU_PWRDN_ST);
	} while ((val & __BIT(cpuno)) != 0);

	delay(2000);

	/* Set wake vector */
	bus_space_write_4(bst, bsh_sram, SRAM_ENTRY_PA, rk3066_mpstart_pa());
	/* Notify boot rom that we are ready to start */
	bus_space_write_4(bst, bsh_sram, SRAM_DOORBELL, SRAM_DOORBELL_MAGIC);
	dsb();
	sev();

	bus_space_unmap(bst, bsh_pmu, sz_pmu);
	bus_space_unmap(bst, bsh_sram, sz_sram);

	return 0;
}

static int
cpu_enable_rk3066(int phandle)
{
	uint64_t mpidr;

	fdtbus_get_reg64(phandle, 0, &mpidr, NULL);

	const u_int cpuno = __SHIFTOUT(mpidr, MPIDR_AFF0);

	cpu_dcache_wbinv_all();

	return rk3066_smp_enable(OF_parent(phandle), cpuno);
}

ARM_CPU_METHOD(rk3066, "rockchip,rk3066-smp", cpu_enable_rk3066);
