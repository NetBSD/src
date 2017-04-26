/* $NetBSD: exynos_fdt.c,v 1.3.2.1 2017/04/26 02:53:01 pgoyette Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_exynos.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_fdt.c,v 1.3.2.1 2017/04/26 02:53:01 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/mainbus/mainbus.h>
#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>

#include <dev/fdt/fdtvar.h>
#include <dev/ofw/openfirm.h>

static int	exynosfdt_match(device_t, cfdata_t, void *);
static void	exynosfdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exynos_fdt, 0,
    exynosfdt_match, exynosfdt_attach, NULL, NULL);

static bool exynosfdt_found = false;

int
exynosfdt_match(device_t parent, cfdata_t cf, void *aux)
{
	if (exynosfdt_found)
		return 0;
	return 1;
}

void
exynosfdt_attach(device_t parent, device_t self, void *aux)
{
	exynosfdt_found = true;

	aprint_naive("\n");
	aprint_normal("\n");

	struct fdt_attach_args faa = {
		.faa_name = "",
		.faa_bst = &armv7_generic_bs_tag,
		.faa_a4x_bst = &armv7_generic_a4x_bs_tag,
		.faa_dmat = &exynos_bus_dma_tag,
		.faa_phandle = OF_peer(0),
	};
	config_found(self, &faa, NULL);
}
