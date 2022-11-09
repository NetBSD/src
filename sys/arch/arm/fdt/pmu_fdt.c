/* $NetBSD: pmu_fdt.c,v 1.11 2022/11/09 19:03:38 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pmu_fdt.c,v 1.11 2022/11/09 19:03:38 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/kmem.h>
#include <sys/xcall.h>

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

static bool	pmu_fdt_uses_ppi;
static int	pmu_fdt_count;

static int	pmu_fdt_match(device_t, cfdata_t, void *);
static void	pmu_fdt_attach(device_t, device_t, void *);

static void	pmu_fdt_init(device_t);
static int	pmu_fdt_intr_distribute(const int, int, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,armv8-pmuv3" },
	{ .compat = "arm,cortex-a73-pmu" },
	{ .compat = "arm,cortex-a72-pmu" },
	{ .compat = "arm,cortex-a57-pmu" },
	{ .compat = "arm,cortex-a53-pmu" },

	{ .compat = "arm,cortex-a35-pmu" },
	{ .compat = "arm,cortex-a17-pmu" },
	{ .compat = "arm,cortex-a12-pmu" },
	{ .compat = "arm,cortex-a9-pmu" },
	{ .compat = "arm,cortex-a8-pmu" },
	{ .compat = "arm,cortex-a7-pmu" },
	{ .compat = "arm,cortex-a5-pmu" },

	DEVICE_COMPAT_EOL
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

	return of_compatible_match(faa->faa_phandle, compat_data);
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
	void **ih;

	if (pmu_fdt_uses_ppi && pmu_fdt_count > 0) {
		/*
		 * Second instance of a PMU where PPIs are used. Since the PMU
		 * is already initialized and the PPI interrupt handler has
		 * already been installed, there is nothing left to do here.
		 */
		if (fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr)))
			aprint_normal_dev(self, "interrupting on %s\n", intrstr);
		return;
	}

	if (pmu_fdt_count == 0) {
		error = arm_pmu_init();
		if (error) {
			aprint_error_dev(self,
			    "couldn't initialise PMU event counter");
			return;
		}
	}

	ih = kmem_zalloc(sizeof(void *) * ncpu, KM_SLEEP);

	for (n = 0; n < ncpu; n++) {
		ih[n] = fdtbus_intr_establish_xname(phandle, n, IPL_HIGH,
		    FDT_INTR_MPSAFE, arm_pmu_intr, NULL, device_xname(self));
		if (ih[n] == NULL)
			break;
		if (!fdtbus_intr_str(phandle, n, intrstr, sizeof(intrstr))) {
			aprint_error_dev(self,
			    "couldn't decode interrupt %u\n", n);
			goto cleanup;
		}
		aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	}

	/* We need either one IRQ (PPI), or one per CPU (SPI) */
	const int nirq = n;
	if (nirq == 0) {
		aprint_error_dev(self, "couldn't establish interrupts\n");
		goto cleanup;
	}

	/* Set interrupt affinity if we have more than one interrupt */
	if (nirq > 1) {
		for (n = 0; n < nirq; n++) {
			error = pmu_fdt_intr_distribute(phandle, n, ih[n]);
			if (error != 0) {
				aprint_error_dev(self,
				    "failed to distribute interrupt %u: %d\n",
				    n, error);
				goto cleanup;
			}
		}
	}

	pmu_fdt_count++;
	pmu_fdt_uses_ppi = nirq == 1 && ncpu > 1;

cleanup:
	kmem_free(ih, sizeof(void *) * ncpu);
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
			    __SHIFTIN(ci->ci_core_id, MPIDR_AFF0) |
			    __SHIFTIN(ci->ci_package_id, MPIDR_AFF1);
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
