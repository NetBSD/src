/*	$NetBSD: exynos_usbphy.c,v 1.1.18.2 2017/12/03 11:35:56 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#include "locators.h"
#include "ohci.h"
#include "ehci.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: exynos_usbphy.c,v 1.1.18.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>

#include <dev/fdt/fdtvar.h>

struct exynos_usbphy_softc {
	device_t	 sc_dev;
	bus_space_tag_t  sc_bst;
	bus_space_handle_t sc_bsh;
};

static int	exynos_usbphy_match(device_t, cfdata_t, void *);
static void	exynos_usbphy_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exynos_usbphy, sizeof(struct exynos_usbphy_softc),
    exynos_usbphy_match, exynos_usbphy_attach, NULL, NULL);


static int
exynos_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5-usb2phy",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}


static void
exynos_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usbphy_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_normal(": USB 2 PHY NOT IMPLEMENTED\n");
}
