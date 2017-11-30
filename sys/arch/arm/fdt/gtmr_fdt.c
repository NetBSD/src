/* $NetBSD: gtmr_fdt.c,v 1.6 2017/11/30 14:50:34 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: gtmr_fdt.c,v 1.6 2017/11/30 14:50:34 skrll Exp $");

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

static int	gtmr_fdt_match(device_t, cfdata_t, void *);
static void	gtmr_fdt_attach(device_t, device_t, void *);

static void	gtmr_fdt_cpu_hatch(void *, struct cpu_info *);

CFATTACH_DECL_NEW(gtmr_fdt, 0, gtmr_fdt_match, gtmr_fdt_attach, NULL, NULL);

/* The vritual timer list entry */
#define GTMR_VTIMER 2

static int
gtmr_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,armv7-timer",
		"arm,armv8-timer",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_compatible(faa->faa_phandle, compatible) >= 0;
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

	void *ih = fdtbus_intr_establish(phandle, GTMR_VTIMER, IPL_CLOCK,
	    FDT_INTR_MPSAFE, gtmr_intr, NULL);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install interrupt handler\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	config_found(self, &mpcaa, NULL);

	arm_fdt_cpu_hatch_register(self, gtmr_fdt_cpu_hatch);
	arm_fdt_timer_register(gtmr_cpu_initclocks);
}

static void
gtmr_fdt_cpu_hatch(void *priv, struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
