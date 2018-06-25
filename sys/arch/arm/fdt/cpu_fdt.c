/* $NetBSD: cpu_fdt.c,v 1.4.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_fdt.c,v 1.4.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

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

	{ "arm,arm-v8",			ARM_CPU_ARMV8 },
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
		/* XXX NetBSD requires all CPUs to be in the same cluster */
		if (fdtbus_get_reg(phandle, 0, &mpidr, NULL) != 0)
			return 0;

		const u_int bp_clid = cpu_clusterid();
		const u_int clid = __SHIFTOUT(mpidr, MPIDR_AFF1);

		if (bp_clid != clid)
			return 0;
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

		cpuid = __SHIFTOUT(mpidr, MPIDR_AFF0);
		break;
	default:
		cpuid = 0;
		break;
	}

	/* Attach the CPU */
	cpu_attach(self, cpuid);
}
