/* $NetBSD: cpu_acpi.c,v 1.14 2022/05/16 09:42:32 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#include "tprof.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_acpi.c,v 1.14 2022/05/16 09:42:32 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/kcpuset.h>
#include <sys/reboot.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/cpuvar.h>
#include <arm/locore.h>

#include <arm/arm/psci.h>

#if NTPROF > 0
#include <dev/tprof/tprof_armv8.h>
#endif

static int	cpu_acpi_match(device_t, cfdata_t, void *);
static void	cpu_acpi_attach(device_t, device_t, void *);

#if NTPROF > 0
static void	cpu_acpi_tprof_init(device_t);
#endif

CFATTACH_DECL_NEW(cpu_acpi, 0, cpu_acpi_match, cpu_acpi_attach, NULL, NULL);

#ifdef MULTIPROCESSOR
static register_t
cpu_acpi_mpstart_pa(void)
{

	return (register_t)KERN_VTOPHYS((vaddr_t)cpu_mpstart);
}
#endif /* MULTIPROCESSOR */

static int
cpu_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_SUBTABLE_HEADER *hdrp = aux;
	ACPI_MADT_GENERIC_INTERRUPT *gicc;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_INTERRUPT)
		return 0;

	gicc = (ACPI_MADT_GENERIC_INTERRUPT *)hdrp;

	return (gicc->Flags & ACPI_MADT_ENABLED) != 0;
}

static void
cpu_acpi_attach(device_t parent, device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);
	ACPI_MADT_GENERIC_INTERRUPT *gicc = aux;
	const uint64_t mpidr = gicc->ArmMpidr;
	const int unit = device_unit(self);
	struct cpu_info *ci = &cpu_info_store[unit];

#ifdef MULTIPROCESSOR
	if (cpu_mpidr_aff_read() != mpidr && (boothowto & RB_MD1) == 0) {
		const u_int cpuindex = device_unit(self);
		int error;

		cpu_mpidr[cpuindex] = mpidr;
		cpu_dcache_wb_range((vaddr_t)&cpu_mpidr[cpuindex],
		    sizeof(cpu_mpidr[cpuindex]));

		/* XXX support spin table */
		error = psci_cpu_on(mpidr, cpu_acpi_mpstart_pa(), 0);
		if (error != PSCI_SUCCESS) {
			aprint_error_dev(self, "failed to start CPU\n");
			return;
		}

		sev();

		for (u_int i = 0x10000000; i > 0; i--) {
			if (cpu_hatched_p(cpuindex))
				 break;
		}
	}
#endif /* MULTIPROCESSOR */

	/* Assume that less efficient processors are faster. */
	prop_dictionary_set_uint32(dict, "capacity_dmips_mhz",
	    gicc->EfficiencyClass);

	/* Store the ACPI Processor UID in cpu_info */
	ci->ci_acpiid = gicc->Uid;

	/* Attach the CPU */
	cpu_attach(self, mpidr);

#if NTPROF > 0
	if (cpu_mpidr_aff_read() == mpidr && armv8_pmu_detect())
		config_interrupts(self, cpu_acpi_tprof_init);
#endif
}

#if NTPROF > 0
static struct cpu_info *
cpu_acpi_find_processor(UINT32 uid)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_acpiid == uid)
			return ci;
	}

	return NULL;
}

static ACPI_STATUS
cpu_acpi_tprof_intr_establish(ACPI_SUBTABLE_HEADER *hdrp, void *aux)
{
	device_t dev = aux;
	ACPI_MADT_GENERIC_INTERRUPT *gicc;
	struct cpu_info *ci;
	char xname[16];
	kcpuset_t *set;
	int error;
	void *ih;

	if (hdrp->Type != ACPI_MADT_TYPE_GENERIC_INTERRUPT)
		return AE_OK;

	gicc = (ACPI_MADT_GENERIC_INTERRUPT *)hdrp;
	if ((gicc->Flags & ACPI_MADT_ENABLED) == 0)
		return AE_OK;

	const bool cpu_primary_p = cpu_info_store[0].ci_cpuid == gicc->ArmMpidr;
	const bool intr_ppi_p = gicc->PerformanceInterrupt < 32;
	const int type = (gicc->Flags & ACPI_MADT_PERFORMANCE_IRQ_MODE) ?
	    IST_EDGE : IST_LEVEL;

	if (intr_ppi_p && !cpu_primary_p)
		return AE_OK;

	ci = cpu_acpi_find_processor(gicc->Uid);
	if (ci == NULL) {
		aprint_error_dev(dev, "couldn't find processor %#x\n",
		    gicc->Uid);
		return AE_OK;
	}

	if (intr_ppi_p) {
		strlcpy(xname, "pmu", sizeof(xname));
	} else {
		snprintf(xname, sizeof(xname), "pmu %s", cpu_name(ci));
	}

	ih = intr_establish_xname(gicc->PerformanceInterrupt, IPL_HIGH,
	    type | IST_MPSAFE, armv8_pmu_intr, NULL, xname);
	if (ih == NULL) {
		aprint_error_dev(dev, "couldn't establish %s interrupt\n",
		    xname);
		return AE_OK;
	}

	if (!intr_ppi_p) {
		kcpuset_create(&set, true);
		kcpuset_set(set, cpu_index(ci));
		error = interrupt_distribute(ih, set, NULL);
		kcpuset_destroy(set);

		if (error) {
			aprint_error_dev(dev,
			    "failed to distribute %s interrupt: %d\n",
			    xname, error);
			return AE_OK;
		}
	}

	aprint_normal("%s: PMU interrupting on irq %d\n", cpu_name(ci),
	    gicc->PerformanceInterrupt);

	return AE_OK;
}

static void
cpu_acpi_tprof_init(device_t self)
{
	int err = armv8_pmu_init();
	if (err) {
		aprint_error_dev(self,
		    "failed to initialize PMU event counter\n");
		return;
	}

	if (acpi_madt_map() != AE_OK) {
		aprint_error_dev(self,
		    "failed to map MADT, performance counters not available\n");
		return;
	}
	acpi_madt_walk(cpu_acpi_tprof_intr_establish, self);
	acpi_madt_unmap();
}
#endif
