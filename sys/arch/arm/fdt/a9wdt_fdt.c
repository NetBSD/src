/* $NetBSD: a9wdt_fdt.c,v 1.4 2021/08/07 16:18:43 thorpej Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: a9wdt_fdt.c,v 1.4 2021/08/07 16:18:43 thorpej Exp $");

#if 0
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/cortex/a9tmr_intr.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/a9tmr_var.h>
#endif


#include <sys/param.h>
#include <sys/bus.h>

#include <arm/cortex/mpcore_var.h>


#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

static int	a9wdt_fdt_match(device_t, cfdata_t, void *);
static void	a9wdt_fdt_attach(device_t, device_t, void *);

struct a9wdt_fdt_softc {
	device_t	sc_dev;
	struct clk	*sc_clk;
};

CFATTACH_DECL_NEW(a9wdt_fdt, sizeof(struct a9wdt_fdt_softc),
    a9wdt_fdt_match, a9wdt_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "arm,cortex-a9-twd-wdt" },
	{ .compat = "arm,cortex-a5-twd-wdt" },
	DEVICE_COMPAT_EOL
};

static int
a9wdt_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
a9wdt_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct a9wdt_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_handle_t bsh;

	sc->sc_dev = self;

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &bsh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "a9wdt",
		.mpcaa_memt = faa->faa_bst,
		.mpcaa_memh = bsh,
		.mpcaa_irq = -1,
	};

	config_found(self, &mpcaa, NULL, CFARGS_NONE);
}

