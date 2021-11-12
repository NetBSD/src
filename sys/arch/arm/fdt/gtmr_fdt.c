/* $NetBSD: gtmr_fdt.c,v 1.12 2021/11/12 21:59:05 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gtmr_fdt.c,v 1.12 2021/11/12 21:59:05 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/cortex/gtmr_intr.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_var.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#if defined(__arm__)
#include <arm/armreg.h>
#endif

static int	gtmr_fdt_match(device_t, cfdata_t, void *);
static void	gtmr_fdt_attach(device_t, device_t, void *);

static void	gtmr_fdt_cpu_hatch(void *, struct cpu_info *);

CFATTACH_DECL_NEW(gtmr_fdt, 0, gtmr_fdt_match, gtmr_fdt_attach, NULL, NULL);

/* The virtual timer list entry */
#define GTMR_VTIMER 2

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,armv7-timer" },
	{ .compat = "arm,armv8-timer" },
	DEVICE_COMPAT_EOL
};

static int
gtmr_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
gtmr_fdt_attach(device_t parent, device_t self, void *aux)
{
	aprint_naive("\n");
	aprint_normal(": Generic Timer\n");

	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "armgtmr",
		.mpcaa_irq = -1		/* setup handler locally */
	};
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, GTMR_VTIMER, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	void *ih = fdtbus_intr_establish_xname(phandle, GTMR_VTIMER, IPL_CLOCK,
	    FDT_INTR_MPSAFE, gtmr_intr, NULL, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

#if defined(__arm__)
	/*
	 * If arm,cpu-registers-not-fw-configured is present, we need
	 * to initialize cntfrq from the clock-frequency property. Only
	 * applicable on Armv7.
	 */
	if (of_hasprop(phandle, "arm,cpu-registers-not-fw-configured")) {
		uint32_t freq;

		if (of_getprop_uint32(phandle, "clock-frequency", &freq) == 0) {
			armreg_cnt_frq_write(freq);
			prop_dictionary_set_bool(device_properties(self),
			    "arm,cpu-registers-not-fw-configured", true);
		}
	}
#endif

	config_found(self, &mpcaa, NULL, CFARGS_NONE);

	arm_fdt_cpu_hatch_register(self, gtmr_fdt_cpu_hatch);
	arm_fdt_timer_register(gtmr_cpu_initclocks);
}

static void
gtmr_fdt_cpu_hatch(void *priv, struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
