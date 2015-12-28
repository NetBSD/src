/*	$NetBSD: exynos_usb.c,v 1.13.2.4 2015/12/28 09:20:24 skrll Exp $	*/

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

__KERNEL_RCSID(1, "$NetBSD: exynos_usb.c,v 1.13.2.4 2015/12/28 09:20:24 skrll Exp $");

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

#if NOHCI > 0
#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>
#endif

#if NEHCI > 0
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#endif

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>

#include <dev/fdt/fdtvar.h>

struct exynos_usb_softc {
	device_t	 sc_self;

	/* keep our tags here */
	bus_dma_tag_t	 sc_dmat;
	bus_space_tag_t  sc_bst;

	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ehci_bsh;
	bus_space_handle_t sc_ohci_bsh;
	bus_space_handle_t sc_usb2phy_bsh;

	device_t	 sc_ohci_dev;
	device_t	 sc_ehci_dev;

	int		 sc_irq;
	void		*sc_intrh;
} exynos_usb_sc;


struct exynos_usb_attach_args {
	const char *name;
};


/* forwards */
//static int exynos_usb_intr(void *arg);


static int	exynos_usb_match(device_t, cfdata_t, void *);
static void	exynos_usb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exyo_usb, 0,
    exynos_usb_match, exynos_usb_attach, NULL, NULL);


static int
exynos_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5-dwusb3",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}


static void
exynos_usb_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc * const sc = &exynos_usb_sc;
	struct fdt_attach_args *const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	/* copy our device handle */
	sc->sc_self = self;
//	sc->sc_irq  = loc->loc_intr;

	/* get our bushandles */
	sc->sc_bst  = faa->faa_bst;
//	sc->sc_dmat = exyoaa->exyo_dmat;
//	sc->sc_dmat = exyoaa->exyo_coherent_dmat;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_normal(" @ 0x%08x: USB -  NOT IMPLEMENTED", (uint)addr);

	aprint_naive("\n");
	aprint_normal("\n");

	/* power up USB subsystem */
//	exynos_usb_soc_powerup();

	/* init USB phys */
//	exynos_usb_phy_init(sc->sc_usb2phy_bsh);

#if 0
	/* claim shared interrupt for OHCI/EHCI */
	sc->sc_intrh = intr_establish(sc->sc_irq,
		IPL_USB, IST_LEVEL, exynos_usb_intr, sc);
	if (!sc->sc_intrh) {
		aprint_error(": unable to establish interrupt at irq %d\n",
			sc->sc_irq);
		/* disable? TBD */
		return;
	}
#endif
	aprint_normal_dev(sc->sc_self, "USB host NOT IMPLEMENTED\n");
}

#if 0
static int
exynos_usb_intr(void *arg)
{
	struct exynos_usb_softc *sc = (struct exynos_usb_softc *) arg;
	void *private;
	int ret = 0;

#if NEHCI > 0
	private = device_private(sc->sc_ehci_dev);
	if (private)
		ret = ehci_intr(private);
#endif
	/* XXX should we always deliver to ohci even if ehci takes it? */
//	if (ret)
//		return ret;
#if NOHCI > 0
	private = device_private(sc->sc_ohci_dev);
	if (private)
		ret = ohci_intr(private);
#endif

	return ret;
}
#endif

#if NOHCI > 0
static int	exynos_ohci_match(device_t, cfdata_t, void *);
static void	exynos_ohci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_exyousb, sizeof(struct ohci_softc),
    exynos_ohci_match, exynos_ohci_attach, NULL, NULL);


static int
exynos_ohci_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5-ohci",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}


static void
exynos_ohci_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc *usbsc = &exynos_usb_sc;
	struct ohci_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->iot = usbsc->sc_bst;
//	sc->ioh = usbsc->sc_ohci_bsh;
	sc->sc_size = size;
	sc->sc_bus.ub_dmatag = usbsc->sc_dmat;
	sc->sc_bus.ub_hcpriv = sc;

	error = bus_space_map(sc->iot, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller NOT IMPLEMENTED\n");

	/* attach */
	int err = ohci_init(sc);
	if (err) {
		aprint_error_dev(self, "init failed, error = %d\n", err);
		/* disable : TBD */
		return;
	}
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
		usbsc->sc_irq);
}
#endif


#if NEHCI > 0
static int	exynos_ehci_match(device_t, cfdata_t, void *);
static void	exynos_ehci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ehci_exyousb, sizeof(struct ehci_softc),
    exynos_ehci_match, exynos_ehci_attach, NULL, NULL);


static int
exynos_ehci_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5-ehci",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}


static void
exynos_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc *usbsc = &exynos_usb_sc;
	struct ehci_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->iot = usbsc->sc_bst;
	sc->ioh = usbsc->sc_ehci_bsh;
	sc->sc_size = size;
	sc->sc_bus.ub_dmatag = usbsc->sc_dmat;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_ncomp = 0;
	if (usbsc->sc_ohci_dev != NULL)
		sc->sc_comps[sc->sc_ncomp++] = usbsc->sc_ohci_dev;

	strlcpy(sc->sc_vendor, "exynos", sizeof(sc->sc_vendor));

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": EHCI USB controller NOT IMPLEMENTED\n");

	/* attach */
	int err = ehci_init(sc);
	if (err) {
		aprint_error_dev(self, "init failed, error = %d\n", err);
		/* disable : TBD */
		return;
	}
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
		usbsc->sc_irq);
}
#endif

