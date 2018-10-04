/* $NetBSD: cpu_fdt.c,v 1.15 2018/10/04 08:58:13 ryo Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_multiprocessor.h"
#include "psci_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_fdt.c,v 1.15 2018/10/04 08:58:13 ryo Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/arm/psci.h>
#include <arm/fdt/arm_fdtvar.h>
#include <arm/fdt/psci_fdtvar.h>

#include <uvm/uvm_extern.h>

static int	cpu_fdt_match(device_t, cfdata_t, void *);
static void	cpu_fdt_attach(device_t, device_t, void *);

enum cpu_fdt_type {
	ARM_CPU_UP = 1,
	ARM_CPU_ARMV7,
	ARM_CPU_ARMV8,
};

struct cpu_fdt_softc {
	device_t		sc_dev;
	int			sc_phandle;
};

static const struct of_compat_data compat_data[] = {
	{ "arm,arm1176jzf-s",		ARM_CPU_UP },

	{ "arm,arm-v7",			ARM_CPU_ARMV7 },
	{ "arm,cortex-a5",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a7",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a8",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a9",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a12",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a15",		ARM_CPU_ARMV7 },
	{ "arm,cortex-a17",		ARM_CPU_ARMV7 },

	{ "arm,armv8",			ARM_CPU_ARMV8 },
	{ "arm,cortex-a53",		ARM_CPU_ARMV8 },
	{ "arm,cortex-a57",		ARM_CPU_ARMV8 },
	{ "arm,cortex-a72",		ARM_CPU_ARMV8 },
	{ "arm,cortex-a73",		ARM_CPU_ARMV8 },

	{ NULL }
};

CFATTACH_DECL_NEW(cpu_fdt, sizeof(struct cpu_fdt_softc),
	cpu_fdt_match, cpu_fdt_attach, NULL, NULL);

static int
cpu_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	enum cpu_fdt_type type;
	int is_compatible;
	bus_addr_t mpidr;

	is_compatible = of_match_compat_data(phandle, compat_data);
	if (!is_compatible)
		return 0;

	type = of_search_compatible(phandle, compat_data)->data;
	switch (type) {
	case ARM_CPU_ARMV7:
	case ARM_CPU_ARMV8:
		if (fdtbus_get_reg(phandle, 0, &mpidr, NULL) != 0)
			return 0;

#ifndef __aarch64__
		/* XXX NetBSD/arm requires all CPUs to be in the same cluster */
		const u_int bp_clid = cpu_clusterid();
		const u_int clid = __SHIFTOUT(mpidr, MPIDR_AFF1);

		if (bp_clid != clid)
			return 0;
#endif
		break;
	default:
		break;
	}

	return is_compatible;
}

static void
cpu_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	enum cpu_fdt_type type;
	bus_addr_t mpidr;
	cpuid_t cpuid;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	type = of_search_compatible(phandle, compat_data)->data;

	switch (type) {
	case ARM_CPU_ARMV7:
	case ARM_CPU_ARMV8:
		if (fdtbus_get_reg(phandle, 0, &mpidr, NULL) != 0) {
			aprint_error(": missing 'reg' property\n");
			return;
		}
#ifndef __aarch64__
		mpidr = __SHIFTOUT(mpidr, MPIDR_AFF0);
#endif
		cpuid = mpidr;
		break;
	default:
		cpuid = 0;
		break;
	}

	/* Attach the CPU */
	cpu_attach(self, cpuid);

	/* Attach CPU frequency scaling provider */
	config_found(self, faa, NULL);
}

#ifdef MULTIPROCESSOR
static register_t
cpu_fdt_mpstart_pa(void)
{
#ifdef __aarch64__
	extern void aarch64_mpstart(void);
	return (register_t)aarch64_kern_vtophys((vaddr_t)aarch64_mpstart);
#else
	extern void cortex_mpstart(void);
	return (register_t)cortex_mpstart;
#endif
}

static int
spintable_cpu_on(u_int cpuindex, paddr_t entry_point_address, paddr_t cpu_release_addr)
{
	/*
	 * we need devmap for cpu-release-addr in advance.
	 * __HAVE_MM_MD_DIRECT_MAPPED_PHYS nor pmap didn't work at this point.
	 */
	if (pmap_devmap_find_pa(cpu_release_addr, sizeof(paddr_t)) == NULL) {
		aprint_error("%s: devmap for cpu-release-addr"
		    " 0x%08"PRIxPADDR" required\n", __func__, cpu_release_addr);
		return -1;
	} else {
		extern struct bus_space arm_generic_bs_tag;
		bus_space_handle_t ioh;

		bus_space_map(&arm_generic_bs_tag, cpu_release_addr,
		    sizeof(paddr_t), 0, &ioh);
		bus_space_write_4(&arm_generic_bs_tag, ioh, 0,
		    entry_point_address);
		bus_space_unmap(&arm_generic_bs_tag, ioh, sizeof(paddr_t));
	}

	return 0;
}
#endif /* MULTIPROCESSOR */

#ifdef MULTIPROCESSOR
static bool
arm_fdt_cpu_okay(const int child)
{
	const char *s;

	s = fdtbus_get_string(child, "device_type");
	if (!s || strcmp(s, "cpu") != 0)
		return false;

	s = fdtbus_get_string(child, "status");
	if (s) {
		if (strcmp(s, "okay") == 0)
			return false;
		if (strcmp(s, "disabled") == 0)
			return of_hasprop(child, "enable-method");
		return false;
	} else {
		return true;
	}
}
#endif /* MULTIPROCESSOR */

void
arm_fdt_cpu_bootstrap(void)
{
#ifdef MULTIPROCESSOR
	uint64_t mpidr, bp_mpidr;
	u_int cpuindex;
	int child, ret;
	const char *method;
#if NPSCI_FDT > 0
	bool nopsci = false;
#endif

	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error("%s: no /cpus node found\n", __func__);
		arm_cpu_max = 1;
		return;
	}

	/* Count CPUs */
	arm_cpu_max = 0;
	for (child = OF_child(cpus); child; child = OF_peer(child))
		if (arm_fdt_cpu_okay(child))
			arm_cpu_max++;

#if NPSCI_FDT > 0
	if (psci_fdt_preinit() != 0)
		nopsci = true;
#endif

	/* MPIDR affinity levels of boot processor. */
	bp_mpidr = cpu_mpidr_aff_read();

	/* Boot APs */
	uint32_t started = 0;
	cpuindex = 1;
	for (child = OF_child(cpus); child; child = OF_peer(child)) {
		if (!arm_fdt_cpu_okay(child))
			continue;
		if (fdtbus_get_reg64(child, 0, &mpidr, NULL) != 0)
			continue;
		if (mpidr == bp_mpidr)
			continue; 	/* BP already started */

#ifdef __arm__
		/* XXX NetBSD/arm requires all CPUs to be in the same cluster */
		if ((mpidr & ~MPIDR_AFF0) != (bp_mpidr & ~MPIDR_AFF0))
			continue;
#endif

#ifdef __aarch64__
		KASSERT(cpuindex < MAXCPUS);
		cpu_mpidr[cpuindex] = mpidr;
		cpu_dcache_wb_range((vaddr_t)&cpu_mpidr[cpuindex], sizeof(cpu_mpidr[cpuindex]));
#endif

		method = fdtbus_get_string(child, "enable-method");
		if (method == NULL)
			continue;

		if (strcmp(method, "spin-table") == 0) {
			uint64_t data;
			paddr_t cpu_release_addr;

			if (OF_getprop(child, "cpu-release-addr", &data,
			    sizeof(data)) != sizeof(data))
				continue;

			cpu_release_addr = (paddr_t)be64toh(data);
			ret = spintable_cpu_on(mpidr, cpu_fdt_mpstart_pa(), cpu_release_addr);
			if (ret != 0)
				continue;

#if NPSCI_FDT > 0
		} else if (!nopsci && (strcmp(method, "psci") == 0)) {
			ret = psci_cpu_on(mpidr, cpu_fdt_mpstart_pa(), 0);
			if (ret != PSCI_SUCCESS)
				continue;
#endif
		} else {
			aprint_error("%s: %s: unsupported method\n", __func__, method);
			continue;
		}

		started |= __BIT(cpuindex);
		cpuindex++;
	}

	/* Wake up AP in case firmware has placed it in WFE state */
	__asm __volatile("sev" ::: "memory");

	/* Wait for APs to start */
	for (u_int i = 0x10000000; i > 0; i--) {
		membar_consumer();
		if (arm_cpu_hatched == started)
			break;
	}
#endif /* MULTIPROCESSOR */
}
