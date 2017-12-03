/*	$Id: obio_ohci.c,v 1.7.2.4 2017/12/03 11:35:55 jdolecek Exp $	*/

/* adapted from: */
/*	$NetBSD: obio_ohci.c,v 1.7.2.4 2017/12/03 11:35:55 jdolecek Exp $	*/
/*	$OpenBSD: pxa2x0_ohci.c,v 1.19 2005/04/08 02:32:54 dlg Exp $ */

/*
 * Copyright (c) 2005 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "opt_omap.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_ohci.c,v 1.7.2.4 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_obioreg.h>


struct obioohci_softc {
	ohci_softc_t	sc;

	void		*sc_ih;
	bus_addr_t	sc_addr;
	bus_addr_t	sc_size;
};

static int	obioohci_match(device_t, cfdata_t, void *);
static void	obioohci_attach(device_t, device_t, void *);
static int	obioohci_detach(device_t, int);
void *		obioohci_fake_intr_establish(int (*)(void *), void *);
void		obioohci_fake_intr(void);

CFATTACH_DECL_NEW(obioohci, sizeof(struct obioohci_softc),
    obioohci_match, obioohci_attach, obioohci_detach, ohci_activate);

static void	obioohci_clkinit(struct obio_attach_args *);
static void	obioohci_enable(struct obioohci_softc *);
static void	obioohci_disable(struct obioohci_softc *);

#define	HREAD4(sc,r)	bus_space_read_4((sc)->sc.iot, (sc)->sc.ioh, (r))
#define	HWRITE4(sc,r,v)	bus_space_write_4((sc)->sc.iot, (sc)->sc.ioh, (r), (v))

static int
obioohci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == OBIOCF_ADDR_DEFAULT
	    || obio->obio_size == OBIOCF_SIZE_DEFAULT
	    || obio->obio_intr == OBIOCF_INTR_DEFAULT)
		return 0;

#if defined(OMAP_2430) || defined(OMAP_2420)
	if (obio->obio_addr != OHCI1_BASE_2430)
		return 0;
#endif
#if defined(OMAP3) && !defined(OMAP4)
	if (obio->obio_addr != OHCI1_BASE_OMAP3)
		return 0;
#endif
#if defined(OMAP4) || defined(OMAP5)
	if (obio->obio_addr != OHCI1_BASE_OMAP4)
		return 0;
#endif

	obioohci_clkinit(obio);

	return 1;
}

static void
obioohci_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *psc = device_private(parent);
	struct obioohci_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;

	KASSERT(psc->sc_obio_dev == NULL);
	psc->sc_obio_dev = self;

	aprint_naive(": USB Controller\n");
	aprint_normal(": USB Controller\n");

	sc->sc_addr = obio->obio_addr;

	sc->sc.iot = obio->obio_iot;
	sc->sc.sc_dev = self;
	sc->sc.sc_size = obio->obio_size;
	sc->sc.sc_bus.ub_dmatag = obio->obio_dmat;
	sc->sc.sc_bus.ub_hcpriv = &sc->sc;

	/* Map I/O space */
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc.ioh)) {
		aprint_error(": couldn't map memory space\n");
		return;
	}

	/* XXX copied from ohci_pci.c. needed? */
	bus_space_barrier(sc->sc.iot, sc->sc.ioh, 0, sc->sc.sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);

	/* start the usb clock */
#ifdef NOTYET
	pxa2x0_clkman_config(CKEN_USBHC, 1);
#endif
	obioohci_enable(sc);

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	sc->sc_ih = intr_establish(obio->obio_intr, IPL_USB, IST_LEVEL,
		ohci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		goto free_map;
	}

	strlcpy(sc->sc.sc_vendor, "OMAP", sizeof(sc->sc.sc_vendor));
	int err = ohci_init(&sc->sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		goto free_intr;
	}

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
	return;

free_intr:
	sc->sc_ih = NULL;
free_map:
	obioohci_disable(sc);
#ifdef NOTYET
	pxa2x0_clkman_config(CKEN_USBHC, 0);
#endif
	bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
	sc->sc.sc_size = 0;
}

static int
obioohci_detach(device_t self, int flags)
{
	struct obioohci_softc *sc = device_private(self);
	int error;

	error = ohci_detach(&sc->sc, flags);
	if (error)
		return error;

	if (sc->sc_ih) {
#ifdef NOTYET
		intr_disestablish(sc->sc_ih);
#endif
		sc->sc_ih = NULL;
	}

	obioohci_disable(sc);

#ifdef NOTYET
	/* stop clock */
#ifdef DEBUG
	pxa2x0_clkman_config(CKEN_USBHC, 0);
#endif
#endif

	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}

	return 0;
}

static void
obioohci_enable(struct obioohci_softc *sc)
{
#ifdef NOTYET
	printf("%s: TBD\n", __func__);
#endif
}

static void
obioohci_disable(struct obioohci_softc *sc)
{
#ifdef NOTYET
	uint32_t hr;

	/* Full host reset */
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) | USBHC_HR_FHR);

	DELAY(USBHC_RST_WAIT);

	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) & ~(USBHC_HR_FHR));
#endif
}

static void
obioohci_clkinit(struct obio_attach_args *obio)
{
#if defined(OMAP_2430) || defined(OMAP_2420)
	bus_space_handle_t ioh;
	uint32_t r;
	int err;

	err = bus_space_map(obio->obio_iot, OMAP2_CM_BASE,
		OMAP2_CM_SIZE, 0, &ioh);
	if (err != 0)
		panic("%s: cannot map OMAP2_CM_BASE at %#x, error %d\n",
			__func__, OMAP2_CM_BASE, err);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN2_CORE);
	r |= OMAP2_CM_FCLKEN2_CORE_EN_USB;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN2_CORE, r);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN2_CORE);
	r |= OMAP2_CM_ICLKEN2_CORE_EN_USB;
	r &= ~OMAP2_CM_ICLKEN2_CORE_EN_USBHS;	/* force FS for ohci */
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN2_CORE, r);

	bus_space_unmap(obio->obio_iot, ioh, OMAP2_CM_SIZE);
#endif
}
