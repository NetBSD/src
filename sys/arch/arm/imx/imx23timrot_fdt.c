/* $NetBSD: imx23timrot_fdt.c,v 1.2 2026/01/13 10:06:58 yurix Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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
__KERNEL_RCSID(0, "$NetBSD: imx23timrot_fdt.c,v 1.2 2026/01/13 10:06:58 yurix Exp $");

#include <sys/param.h>

#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx23var.h>
#include <arm/imx/imx23_timrotvar.h>
#include <arm/fdt/arm_fdtvar.h>

static int imx23timrot_fdt_match(device_t, cfdata_t, void *);
static void imx23timrot_fdt_attach(device_t, device_t, void *);

struct imx23timrot_fdt_softc {
	struct timrot_softc systimer_sc;
	struct timrot_softc stattimer_sc;
};

CFATTACH_DECL_NEW(imx23timrot_fdt, sizeof(struct imx23timrot_fdt_softc),
		  imx23timrot_fdt_match, imx23timrot_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-timrot" },
	{ .compat = "fsl,timrot" },
	DEVICE_COMPAT_EOL
};

static int
imx23timrot_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

/*
 * The original non-FDT timrot driver instantiates one timrot device per timer.
 * Unfortunately, the device tree from linux specifies just one timrot device
 * for all timers. This driver therefore attaches "two" timrot devices from one.
 */
static void
imx23timrot_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct imx23timrot_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];

	/*
	 * The actual memory mapping is done inside the
	 * timrot_[sys|stat]timer_init functions.
	 */
	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}

	/* Use -1 as irq because we establish interrupts ourselves */
	imx23timrot_systimer_init(&sc->systimer_sc, faa->faa_bst, -1);
	imx23timrot_stattimer_init(&sc->stattimer_sc, faa->faa_bst, -1);


	/* System Timer Interrupt*/
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_CLOCK, 0,
					       imx23timrot_systimer_irq, NULL,
					       device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self,
		    "couldn't install systimer interrupt handler\n");
		return;
	}
	aprint_normal(": systimer on %s", intrstr);

	/* Stat Timer Interrupt*/
	if (!fdtbus_intr_str(phandle, 1, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	ih = fdtbus_intr_establish_xname(phandle, 1, IPL_CLOCK, 0,
					 imx23timrot_stattimer_irq, NULL,
					 device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self,
		    "couldn't install stattimer interrupt handler\n");
		return;
	}
	aprint_normal(": stattimer on %s\n", intrstr);

	struct apb_attach_args apbaa = {
		.aa_name = "imx23timrot",
		.aa_iot = faa->faa_bst,
		.aa_dmat = faa->faa_dmat,
		.aa_addr = addr,
		.aa_size = size,
		.aa_irq = -1
	};

	config_found(self, &apbaa, NULL, CFARGS_NONE);

	arm_fdt_timer_register(imx23timrot_cpu_initclocks);
}
