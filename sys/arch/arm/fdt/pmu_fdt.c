/* $NetBSD: pmu_fdt.c,v 1.3.2.2 2018/07/28 04:37:28 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: pmu_fdt.c,v 1.3.2.2 2018/07/28 04:37:28 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>

#include <dev/fdt/fdtvar.h>

#if defined(__aarch64__)
#include <dev/tprof/tprof_armv8.h>
#define arm_pmu_intr armv8_pmu_intr
#define arm_pmu_init armv8_pmu_init
#elif defined(_ARM_ARCH_7)
#include <dev/tprof/tprof_armv7.h>
#define arm_pmu_intr armv7_pmu_intr
#define arm_pmu_init armv7_pmu_init
#endif

#include <arm/armreg.h>

static int	pmu_fdt_match(device_t, cfdata_t, void *);
static void	pmu_fdt_attach(device_t, device_t, void *);

static void	pmu_fdt_init(device_t);
static int	pmu_fdt_intr_distribute(const int, int, void *);

static const char * const compatible[] = {
	"arm,armv8-pmuv3",
	"arm,cortex-a73-pmu",
	"arm,cortex-a72-pmu",
	"arm,cortex-a57-pmu",
	"arm,cortex-a53-pmu",

	"arm,cortex-a35-pmu",
	"arm,cortex-a17-pmu",
	"arm,cortex-a12-pmu",
	"arm,cortex-a9-pmu",
	"arm,cortex-a8-pmu",
	"arm,cortex-a7-pmu",
	"arm,cortex-a5-pmu",

	NULL
};

struct pmu_fdt_softc {
	device_t	sc_dev;
	int		sc_phandle;
};

CFATTACH_DECL_NEW(pmu_fdt, sizeof(struct pmu_fdt_softc),
    pmu_fdt_match, pmu_fdt_attach, NULL, NULL);

static int
pmu_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
pmu_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct pmu_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": Performance Monitor Unit\n");

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	config_interrupts(self, pmu_fdt_init);
}

static void
pmu_fdt_init(device_t self)
{
	struct pmu_fdt_softc * const sc = device_private(self);
	const int phandle = sc->sc_phandle;
	char intrstr[128];
	int error, n;
	void *ih;

	for (n = 0; ; n++) {
		ih = fdtbus_intr_establish(phandle, n, IPL_HIGH,
		    FDT_INTR_MPSAFE, arm_pmu_intr, NULL);
		if (ih == NULL)
			break;
		if (!fdtbus_intr_str(phandle, n, intrstr, sizeof(intrstr))) {
			aprint_error_dev(self,
			    "couldn't decode interrupt %u\n", n);
			return;
		}
		aprint_normal_dev(self, "interrupting on %s\n", intrstr);
		error = pmu_fdt_intr_distribute(phandle, n, ih);
		if (error != 0) {
			aprint_error_dev(self,
			    "failed to distribute interrupt %u: %d\n",
			    n, error);
			return;
		}
	}
	/* We need either one IRQ (PPI), or one per CPU (SPI) */
	if (n == 0) {
		aprint_error_dev(self, "couldn't establish interrupts\n");
		return;
	}

	error = arm_pmu_init();
	if (error != 0) {
		aprint_error_dev(self, "failed to initialize PMU\n");
		return;
	}
}

static int
pmu_fdt_intr_distribute(const int phandle, int index, void *ih)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	bus_addr_t mpidr;
	int len, cpunode;
	const u_int *aff;
	kcpuset_t *set;
	int error;

	kcpuset_create(&set, true);

	if (of_hasprop(phandle, "interrupt-affinity")) {
		aff = fdtbus_get_prop(phandle, "interrupt-affinity", &len);
		if (len < (index + 1) * 4)
			return EINVAL;
		cpunode = fdtbus_get_phandle_from_native(be32toh(aff[index]));
		if (fdtbus_get_reg(cpunode, 0, &mpidr, NULL) != 0)
			return ENXIO;
		for (CPU_INFO_FOREACH(cii, ci)) {
			const uint32_t ci_mpidr =
			    __SHIFTIN(ci->ci_data.cpu_core_id, MPIDR_AFF0) |
			    __SHIFTIN(ci->ci_data.cpu_package_id, MPIDR_AFF1);
			if (ci_mpidr == mpidr) {
				kcpuset_set(set, cpu_index(ci));
				break;
			}
		}
	} else {
		kcpuset_set(set, index);
	}

	if (kcpuset_iszero(set)) {
		kcpuset_destroy(set);
		return ENOENT;
	}

	error = interrupt_distribute(ih, set, NULL);

	kcpuset_destroy(set);

	return error;
}
