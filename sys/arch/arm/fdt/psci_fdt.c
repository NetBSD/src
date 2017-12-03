/* $NetBSD: psci_fdt.c,v 1.3.2.2 2017/12/03 11:35:52 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: psci_fdt.c,v 1.3.2.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <arm/locore.h>
#include <arm/armreg.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdt.h>

static int	psci_fdt_match(device_t, cfdata_t, void *);
static void	psci_fdt_attach(device_t, device_t, void *);

static int	psci_fdt_init(const int);

static const char * const compatible[] = {
	"arm,psci",
	"arm,psci-0.2",
	"arm,psci-1.0",
	NULL
};

CFATTACH_DECL_NEW(psci_fdt, 0, psci_fdt_match, psci_fdt_attach, NULL, NULL);

static void
psci_fdt_reset(device_t dev)
{
	delay(500000);
	psci_system_reset();
}

static void
psci_fdt_poweroff(device_t dev)
{
	delay(500000);
	psci_system_off();
}

static const struct fdtbus_power_controller_func psci_power_funcs = {
	.reset = psci_fdt_reset,
	.poweroff = psci_fdt_poweroff,
};

static int
psci_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
psci_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	psci_fdt_init(phandle);

	const uint32_t ver = psci_version();
	const u_int ver_maj = __SHIFTOUT(ver, PSCI_VERSION_MAJOR);
	const u_int ver_min = __SHIFTOUT(ver, PSCI_VERSION_MINOR);

	aprint_naive("\n");
	aprint_normal(": PSCI %u.%u\n", ver_maj, ver_min);

	fdtbus_register_power_controller(self, phandle,
	    &psci_power_funcs);
}

static int
psci_fdt_init(const int phandle)
{
	char method[4];
	uint32_t val;

	if (!of_hasprop(phandle, "method")) {
		aprint_error("PSCI: missing 'method' property\n");
		return EINVAL;
	}

	OF_getprop(phandle, "method", method, sizeof(method));
	if (strcmp(method, "smc") == 0)
		psci_init(psci_call_smc);
	else if (strcmp(method, "hvc") == 0)
		psci_init(psci_call_hvc);
	else {
		aprint_error("PSCI: unsupported method '%s'\n", method);
		return EINVAL;
	}

	const char * const compat_0_1[] = { "arm,psci", NULL };
	if (of_match_compatible(phandle, compat_0_1)) {
		psci_clearfunc();
		if (of_getprop_uint32(phandle, "cpu_on", &val) == 0)
			psci_setfunc(PSCI_FUNC_CPU_ON, val);
	}

	return 0;
}

void
psci_fdt_bootstrap(void)
{
#ifdef MULTIPROCESSOR
	extern void cortex_mpstart(void);
	bus_addr_t mpidr;
	uint32_t bp_mpidr;
	int child;

	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error("PSCI: no /cpus node found\n");
		arm_cpu_max = 1;
		return;
	}

	/* Count CPUs */
	arm_cpu_max = 0;
	for (child = OF_child(cpus); child; child = OF_peer(child))
		if (fdtbus_status_okay(child))
			arm_cpu_max++;

	const int phandle = OF_finddevice("/psci");
	if (phandle == -1) {
		aprint_error("PSCI: no /psci node found\n");
		return;
	}

	if (psci_fdt_init(phandle) != 0)
		return;

	/* MPIDR affinity levels of boot processor. */
	bp_mpidr = armreg_mpidr_read() & (MPIDR_AFF2|MPIDR_AFF1|MPIDR_AFF0);

	/* Boot APs */
	uint32_t started = 0;
	for (child = OF_child(cpus); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		if (fdtbus_get_reg(child, 0, &mpidr, NULL) != 0)
			continue;
		if (mpidr == bp_mpidr)
			continue; 	/* BP already started */

		/* XXX NetBSD requires all CPUs to be in the same cluster */
		const u_int bp_clid = __SHIFTOUT(bp_mpidr, CORTEXA9_MPIDR_CLID);
		const u_int clid = __SHIFTOUT(mpidr, CORTEXA9_MPIDR_CLID);
		if (bp_clid != clid)
			continue;

		const u_int cpuid = __SHIFTOUT(mpidr, CORTEXA9_MPIDR_CPUID);
		int ret = psci_cpu_on(cpuid, (register_t)cortex_mpstart, 0);
		if (ret == PSCI_SUCCESS)
			started |= __BIT(cpuid);
	}

	/* Wait for APs to start */
	for (u_int i = 0x10000000; i > 0; i--) {
		arm_dmb();
		if (arm_cpu_hatched == started)
			break;
	}
#endif
}
