/* $NetBSD: cpu_acpi.c,v 1.3 2018/10/18 09:01:52 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_acpi.c,v 1.3 2018/10/18 09:01:52 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/arm/psci.h>

static int	cpu_acpi_match(device_t, cfdata_t, void *);
static void	cpu_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu_acpi, 0, cpu_acpi_match, cpu_acpi_attach, NULL, NULL);

static register_t
cpu_acpi_mpstart_pa(void)
{

	return (register_t)KERN_VTOPHYS((vaddr_t)cpu_mpstart);
}

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
	ACPI_MADT_GENERIC_INTERRUPT *gicc = aux;
	const uint64_t mpidr = gicc->ArmMpidr;
	int error;

	if (cpu_mpidr_aff_read() != mpidr) {
		const u_int cpuindex = device_unit(self);

		cpu_mpidr[cpuindex] = mpidr;
		cpu_dcache_wb_range((vaddr_t)&cpu_mpidr[cpuindex], sizeof(cpu_mpidr[cpuindex]));

		/* XXX support spin table */
		error = psci_cpu_on(mpidr, cpu_acpi_mpstart_pa(), 0);
		if (error != PSCI_SUCCESS) {
			aprint_error_dev(self, "failed to start CPU\n");
			return;
		}

		__asm __volatile("sev" ::: "memory");

		for (u_int i = 0x10000000; i > 0; i--) {
			membar_consumer();
			if (arm_cpu_hatched & __BIT(cpuindex))
				break;
		}
	}

	/* Attach the CPU */
	cpu_attach(self, mpidr);
}
