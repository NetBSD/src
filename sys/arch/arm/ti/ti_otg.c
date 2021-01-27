/* $NetBSD: ti_otg.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */
/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_otg.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_otgreg.h>

#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,am33xx-usb" },
	DEVICE_COMPAT_EOL
};

struct tiotg_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	void *			sc_ih;
};

static int	tiotg_match(device_t, cfdata_t, void *);
static void	tiotg_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_otg, sizeof(struct tiotg_softc),
    tiotg_match, tiotg_attach, NULL, NULL);

static int
tiotg_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
tiotg_attach(device_t parent, device_t self, void *aux)
{
	struct tiotg_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t val;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_dev = self;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_normal(": TI dual-port USB controller");

#if 0
	/* XXX this looks wrong */
	prcm_write_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKDCOLDO_DPLL_PER,
	    CM_WKUP_CM_CLKDCOLDO_DPLL_PER_CLKDCOLDO_GATE_CTRL|
	    CM_WKUP_CM_CLKDCOLDO_DPLL_PER_CLKDCOLDO_ST);

	prcm_write_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL, 2);
	while ((prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL) & 0x3) != 2)
		delay(10);

	while (prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL) & (3<<16))
		delay(10);
#endif

	/* reset module */
	TIOTG_USBSS_WRITE4(sc, USBSS_SYSCONFIG, USBSS_SYSCONFIG_SRESET);
	while (TIOTG_USBSS_READ4(sc, USBSS_SYSCONFIG) & USBSS_SYSCONFIG_SRESET)
		delay(10);
	val = TIOTG_USBSS_READ4(sc, USBSS_REVREG);
	aprint_normal(": version v%d.%d.%d.%d",
	    (val >> 11) & 15, (val >> 8) & 7, (val >> 6) & 3, val & 63);
	aprint_normal("\n");

	/* enable clock */
	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	fdt_add_bus(self, phandle, faa);
}
