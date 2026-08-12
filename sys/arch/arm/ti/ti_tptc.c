/* $NetBSD: ti_tptc.c,v 1.3 2026/08/12 09:22:58 yurix Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ti_tptc.c,v 1.3 2026/08/12 09:22:58 yurix Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

static int	ti_tptc_match(device_t, cfdata_t, void *);
static void	ti_tptc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_tptc, 0, ti_tptc_match, ti_tptc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,edma3-tptc" },
	DEVICE_COMPAT_EOL
};

static int
ti_tptc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_tptc_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	if (of_hasprop(phandle, "power-domains")) {
		/* clocks are configured through fdt_powerdomain */
		if (fdtbus_powerdomain_enable(phandle) != 0) {
			aprint_error(": couldn't enable powerdomain\n");
			return;
		}
	} else {
		/* clock are configured through the prcm system  */
		if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
			aprint_error(": couldn't enable module\n");
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": EDMA Transfer Controller\n");
}
