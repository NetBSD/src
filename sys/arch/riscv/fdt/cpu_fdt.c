/* $NetBSD: cpu_fdt.c,v 1.2 2023/06/12 19:04:13 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_fdt.c,v 1.2 2023/06/12 19:04:13 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/cpufunc.h>
#include <riscv/cpuvar.h>
#include <riscv/machdep.h>
#include <riscv/sbi.h>

#include <riscv/fdt/riscv_fdtvar.h>


#ifdef MULTIPROCESSOR
static bool
riscv_fdt_cpu_okay(const int child)
{
	const char *s;

	s = fdtbus_get_string(child, "device_type");
	if (!s || strcmp(s, "cpu") != 0)
		return false;

	s = fdtbus_get_string(child, "status");
	if (s) {
		if (strcmp(s, "okay") == 0)
			return true;
		if (strcmp(s, "disabled") == 0)
			return false;
		return false;
	} else {
		return true;
	}
}
#endif /* MULTIPROCESSOR */

void
riscv_fdt_cpu_bootstrap(void)
{
#ifdef MULTIPROCESSOR

	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error("%s: no /cpus node found\n", __func__);
		riscv_cpu_max = 1;
		return;
	}

	/* Count harts and add hart IDs to to cpu_hartid array */
	size_t cpuindex = 1;
	for (int child = OF_child(cpus); child; child = OF_peer(child)) {
		if (!riscv_fdt_cpu_okay(child))
			continue;

		riscv_cpu_max++;

		uint64_t reg;
		if (fdtbus_get_reg64(child, 0, &reg, NULL) != 0)
			continue;

		const cpuid_t hartid = reg;

		struct sbiret sbiret = sbi_hart_get_status(hartid);
		switch (sbiret.error) {
		case SBI_ERR_INVALID_PARAM:
			aprint_error("Unknown hart id %lx", hartid);
			continue;
		case SBI_SUCCESS:
			break;
		default:
			aprint_error("Unexpected error (%ld) from get_status",
			    sbiret.error);
		}

		/* Assume the BP is the only one started. */
		if (sbiret.value == SBI_HART_STARTED) {
			if (cpu_hartid[0] != -1) {
				panic("more than 1 hart started");
			}
			cpu_hartid[0] = hartid;
			continue;
		}

		KASSERT(cpuindex < MAXCPUS);
		cpu_hartid[cpuindex] = hartid;
		cpu_dcache_wb_range((vaddr_t)&cpu_hartid[cpuindex],
		    sizeof(cpu_hartid[cpuindex]));

		cpuindex++;
	}
#endif
}
int
riscv_fdt_cpu_mpstart(void)
{
	int ret = 0;
#ifdef MULTIPROCESSOR
	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error("%s: no /cpus node found\n", __func__);
		return 0;
	}

	// riscv_fdt_cpu_bootstrap put the boot hart id in cpu_hartid[0]
	const cpuid_t bp_hartid = cpu_hartid[0];

	/* BootAPs */
	size_t cpuindex = 1;
	for (int child = OF_child(cpus); child; child = OF_peer(child)) {
		if (!riscv_fdt_cpu_okay(child))
			continue;

		uint64_t reg;
		if (fdtbus_get_reg64(child, 0, &reg, NULL) != 0)
			continue;

		cpuid_t hartid = reg;

		if (hartid == bp_hartid)
			continue;		/* BP already started */

		const paddr_t entry = KERN_VTOPHYS(cpu_mpstart);
		struct sbiret sbiret = sbi_hart_start(hartid, entry, 0);
		switch (sbiret.error) {
		case SBI_SUCCESS:
			break;
		case SBI_ERR_INVALID_ADDRESS:
			break;
		case SBI_ERR_INVALID_PARAM:
			break;
		case SBI_ERR_ALREADY_AVAILABLE:
			break;
		case SBI_ERR_FAILED:
			break;
		default:
			aprint_error("%s: failed to enable CPU %#lx\n",
			    __func__, hartid);
		}

		size_t i;
		/* Wait for AP to start */
		for (i = 0x10000000; i > 0; i--) {
			if (cpu_hatched_p(cpuindex))
				break;
		}

		if (i == 0) {
			ret++;
			aprint_error("cpu%zu: WARNING: AP failed to start\n",
			    cpuindex);
		}

		cpuindex++;
	}
#else
	aprint_normal("%s: kernel compiled without MULTIPROCESSOR\n", __func__);
#endif /* MULTIPROCESSOR */
	return ret;
}

static int
cpu_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *device_type;

	device_type = fdtbus_get_string(phandle, "device_type");
	return device_type != NULL && strcmp(device_type, "cpu") == 0;
}

static void
cpu_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t cpuid;

	if (fdtbus_get_reg(phandle, 0, &cpuid, NULL) != 0)
		cpuid = 0;

	/* Attach the CPU */
	cpu_attach(self, cpuid);

	fdt_add_bus(self, phandle, faa);
}

CFATTACH_DECL_NEW(cpu_fdt, 0, cpu_fdt_match, cpu_fdt_attach, NULL, NULL);
