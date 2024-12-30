/* $NetBSD: cpu_acpi.c,v 1.17 2024/12/30 19:17:21 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpu_acpi.c,v 1.17 2024/12/30 19:17:21 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/kcpuset.h>
#include <sys/kmem.h>
#include <sys/reboot.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_srat.h>
#include <external/bsd/acpica/dist/include/amlresrc.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/cpuvar.h>
#include <arm/locore.h>

#include <arm/arm/psci.h>

#define LPI_IDLE_FACTOR		3

#if NTPROF > 0
#include <dev/tprof/tprof_armv8.h>
#endif

static int	cpu_acpi_match(device_t, cfdata_t, void *);
static void	cpu_acpi_attach(device_t, device_t, void *);

static void	cpu_acpi_probe_lpi(device_t, struct cpu_info *ci);
void		cpu_acpi_lpi_idle(void);

#if NTPROF > 0
static void	cpu_acpi_tprof_init(device_t);
#endif

CFATTACH_DECL2_NEW(cpu_acpi, 0,
    cpu_acpi_match, cpu_acpi_attach, NULL, NULL,
    cpu_rescan, cpu_childdetached);

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
	struct acpisrat_node *node;

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

	/* Scan SRAT for NUMA info. */
	if (cpu_mpidr_aff_read() == mpidr) {
		acpisrat_init();
	}
	node = acpisrat_get_node(gicc->Uid);
	if (node != NULL) {
		ci->ci_numa_id = node->nodeid;
	}

	/* Attach the CPU */
	cpu_attach(self, mpidr);

	/* Probe for low-power idle states. */
	cpu_acpi_probe_lpi(self, ci);

#if NTPROF > 0
	if (cpu_mpidr_aff_read() == mpidr && armv8_pmu_detect())
		config_interrupts(self, cpu_acpi_tprof_init);
#endif
}

static void
cpu_acpi_probe_lpi(device_t dev, struct cpu_info *ci)
{
	ACPI_HANDLE hdl;
	ACPI_BUFFER buf;
	ACPI_OBJECT *obj, *lpi;
	ACPI_STATUS rv;
	uint32_t levelid;
	uint32_t numlpi;
	uint32_t n;
	int enable_lpi;

	if (get_bootconf_option(boot_args, "nolpi",
				BOOTOPT_TYPE_BOOLEAN, &enable_lpi) &&
	    !enable_lpi) {
		return;
	}

	hdl = acpi_match_cpu_info(ci);
	if (hdl == NULL) {
		return;
	}
	rv = AcpiGetHandle(hdl, "_LPI", &hdl);
	if (ACPI_FAILURE(rv)) {
		return;
	}
	rv = acpi_eval_struct(hdl, NULL, &buf);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	obj = buf.Pointer;
	if (obj->Type != ACPI_TYPE_PACKAGE ||
	    obj->Package.Count < 3 ||
	    obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER ||
	    obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER) {
		goto out;
	}
	levelid = obj->Package.Elements[1].Integer.Value;
	if (levelid != 0) {
		/* We depend on platform coordination for now. */
		goto out;
	}
	numlpi = obj->Package.Elements[2].Integer.Value;
	if (obj->Package.Count < 3 + numlpi || numlpi == 0) {
		goto out;
	}
	ci->ci_lpi = kmem_zalloc(sizeof(*ci->ci_lpi) * numlpi, KM_SLEEP);
	for (n = 0; n < numlpi; n++) {
		lpi = &obj->Package.Elements[3 + n];
		if (lpi->Type != ACPI_TYPE_PACKAGE ||
		    lpi->Package.Count < 10 ||
		    lpi->Package.Elements[0].Type != ACPI_TYPE_INTEGER ||
		    lpi->Package.Elements[1].Type != ACPI_TYPE_INTEGER ||
		    lpi->Package.Elements[2].Type != ACPI_TYPE_INTEGER ||
		    lpi->Package.Elements[3].Type != ACPI_TYPE_INTEGER ||
		    !(lpi->Package.Elements[6].Type == ACPI_TYPE_BUFFER ||
		      lpi->Package.Elements[6].Type == ACPI_TYPE_INTEGER)) {
			continue;
		}

		if ((lpi->Package.Elements[2].Integer.Value & 1) == 0) {
			/* LPI state is not enabled */
			continue;
		}

		ci->ci_lpi[ci->ci_nlpi].min_res
		    = lpi->Package.Elements[0].Integer.Value;
		ci->ci_lpi[ci->ci_nlpi].wakeup_latency =
		    lpi->Package.Elements[1].Integer.Value;
		ci->ci_lpi[ci->ci_nlpi].save_restore_flags =
		    lpi->Package.Elements[3].Integer.Value;
		if (ci->ci_lpi[ci->ci_nlpi].save_restore_flags != 0) {
			/* Not implemented yet */
			continue;
		}
		if (lpi->Package.Elements[6].Type == ACPI_TYPE_INTEGER) {
			ci->ci_lpi[ci->ci_nlpi].reg_addr =
			    lpi->Package.Elements[6].Integer.Value;
		} else {
			ACPI_GENERIC_ADDRESS addr;

			KASSERT(lpi->Package.Elements[6].Type ==
				ACPI_TYPE_BUFFER);

			if (lpi->Package.Elements[6].Buffer.Length <
			    sizeof(AML_RESOURCE_GENERIC_REGISTER)) {
				continue;
			}
			memcpy(&addr, lpi->Package.Elements[6].Buffer.Pointer +
			    sizeof(AML_RESOURCE_LARGE_HEADER), sizeof(addr));
			ci->ci_lpi[ci->ci_nlpi].reg_addr = addr.Address;
		}

		if (lpi->Package.Elements[9].Type == ACPI_TYPE_STRING) {
			ci->ci_lpi[ci->ci_nlpi].name =
			    kmem_asprintf("LPI state %s",
				lpi->Package.Elements[9].String.Pointer);
		} else {
			ci->ci_lpi[ci->ci_nlpi].name =
			    kmem_asprintf("LPI state %u", n + 1);
		}

		aprint_verbose_dev(ci->ci_dev,
		    "%s: min res %u, wakeup latency %u, flags %#x, "
		    "register %#x\n",
		    ci->ci_lpi[ci->ci_nlpi].name,
		    ci->ci_lpi[ci->ci_nlpi].min_res,
		    ci->ci_lpi[ci->ci_nlpi].wakeup_latency,
		    ci->ci_lpi[ci->ci_nlpi].save_restore_flags,
		    ci->ci_lpi[ci->ci_nlpi].reg_addr);

		evcnt_attach_dynamic(&ci->ci_lpi[ci->ci_nlpi].events,
		    EVCNT_TYPE_MISC, NULL, ci->ci_cpuname,
		    ci->ci_lpi[ci->ci_nlpi].name);

		ci->ci_nlpi++;
	}

	if (ci->ci_nlpi > 0) {
		extern void (*arm_cpu_idle)(void);
		arm_cpu_idle = cpu_acpi_lpi_idle;
	}

out:
	ACPI_FREE(buf.Pointer);
}

static inline void
cpu_acpi_idle(uint32_t addr)
{
	if (addr == LPI_REG_ADDR_WFI) {
		asm volatile("dsb sy; wfi");
	} else {
		psci_cpu_suspend(addr);
	}
}

void
cpu_acpi_lpi_idle(void)
{
	struct cpu_info *ci = curcpu();
	struct timeval start, end;
	int n;

	DISABLE_INTERRUPT();

	microuptime(&start);
	for (n = ci->ci_nlpi - 1; n >= 0; n--) {
		if (ci->ci_last_idle >
		    LPI_IDLE_FACTOR * ci->ci_lpi[n].min_res) {
			cpu_acpi_idle(ci->ci_lpi[n].reg_addr);
			ci->ci_lpi[n].events.ev_count++;
			break;
		}
	}
	if (n == -1) {
		/* Nothing in _LPI, let's just WFI. */
		cpu_acpi_idle(LPI_REG_ADDR_WFI);
	}
	microuptime(&end);
	timersub(&end, &start, &end);

	ci->ci_last_idle = end.tv_sec * 1000000 + end.tv_usec;

	ENABLE_INTERRUPT();
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
