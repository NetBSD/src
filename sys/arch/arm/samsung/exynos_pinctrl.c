/*	$NetBSD: exynos_pinctrl.c,v 1.1 2015/12/19 21:42:31 marty Exp $ */

/*-
* Copyright (c) 2015 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Marty Fouts
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_pinctrl.c,v 1.1 2015/12/19 21:42:31 marty Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_io.h>
#include <arm/samsung/exynos_intr.h>
#include <arm/samsung/exynos_pinctrl.h>

#include <dev/fdt/fdtvar.h>

static int exynos_pinctrl_match(device_t, cfdata_t, void *);
static void exynos_pinctrl_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exynos_pinctrl, sizeof(struct exynos_pinctrl_softc),
	exynos_pinctrl_match, exynos_pinctrl_attach, NULL, NULL);

static int
exynos_pinctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5422-pinctrl",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_pinctrl_softc * const sc
		= kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	int child;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	printf(" pinctl @ 0x%08x\n", (uint)addr);
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	for (child = OF_child(faa->faa_phandle); child;
	     child = OF_peer(child)) {
		char result[64];
		error = OF_getprop(child, "gpio-controller", result,
				   sizeof(result));
		if (error == -1)
			continue;
		exynos_gpio_bank_config(sc,child);
	}

	aprint_naive("\n");
	aprint_normal("\n");

}
