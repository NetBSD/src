/*	$NetBSD: pxa2x0_ohci.c,v 1.6 2009/08/09 06:12:34 kiyohara Exp $	*/
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

struct pxaohci_softc {
	ohci_softc_t	sc;

	void		*sc_ih;
};

#if 0
static void	pxaohci_power(int, void *);
#endif
static void	pxaohci_enable(struct pxaohci_softc *);
static void	pxaohci_disable(struct pxaohci_softc *);

#define	HREAD4(sc,r)	bus_space_read_4((sc)->sc.iot, (sc)->sc.ioh, (r))
#define	HWRITE4(sc,r,v)	bus_space_write_4((sc)->sc.iot, (sc)->sc.ioh, (r), (v))

static int
pxaohci_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (CPU_IS_PXA270 && strcmp(pxa->pxa_name, cf->cf_name) == 0) {
		pxa->pxa_size = PXA2X0_USBHC_SIZE;
		return 1;
	}
	return 0;
}

static void
pxaohci_attach(device_t parent, device_t self, void *aux)
{
	struct pxaohci_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	bus_space_handle_t powman_ioh;
	usbd_status r;

#ifdef USB_DEBUG
	{
		//extern int ohcidebug;
		//ohcidebug = 16;
	}
#endif

	sc->sc.iot = pxa->pxa_iot;
	sc->sc.sc_bus.dmatag = pxa->pxa_dmat;
	sc->sc.sc_size = 0;
	sc->sc_ih = NULL;
	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = sc;

	aprint_normal("\n");
	aprint_naive("\n");

	/* Map I/O space */
	if (bus_space_map(sc->sc.iot, pxa->pxa_addr, pxa->pxa_size, 0,
	    &sc->sc.ioh)) {
		aprint_error_dev(sc->sc.sc_dev, "couldn't map memory space\n");
		return;
	}
	sc->sc.sc_size = pxa->pxa_size;

	/* XXX copied from ohci_pci.c. needed? */
	bus_space_barrier(sc->sc.iot, sc->sc.ioh, 0, sc->sc.sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);

	/* start the usb clock */
	pxa2x0_clkman_config(CKEN_USBHC, 1);
	pxaohci_enable(sc);

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_MIE);

	sc->sc_ih = pxa2x0_intr_establish(PXA2X0_INT_USBH1, IPL_USB,
	    ohci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc.sc_dev,
		    "unable to establish interrupt\n");
		goto free_map;
	}

	strlcpy(sc->sc.sc_vendor, "PXA27x", sizeof(sc->sc.sc_vendor));
	r = ohci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(sc->sc.sc_dev, "init failed, error=%d\n", r);
		goto free_intr;
	}

#if 0
	sc->sc.sc_powerhook = powerhook_establish(sc->sc.sc_bus.bdev.dv_xname,
	    pxaohci_power, sc);
	if (sc->sc.sc_powerhook == NULL) {
		aprint_error("%s: cannot establish powerhook\n",
		    sc->sc.sc_dev->sc_bus.bdev.dv_xname);
	}
#endif

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);

	return;

free_intr:
	pxa2x0_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
free_map:
	pxaohci_disable(sc);
	pxa2x0_clkman_config(CKEN_USBHC, 0);
	bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
	sc->sc.sc_size = 0;
}

static int
pxaohci_detach(device_t self, int flags)
{
	struct pxaohci_softc *sc = device_private(self);
	int error;

	error = ohci_detach(&sc->sc, flags);
	if (error)
		return error;

#if 0
	if (sc->sc.sc_powerhook) {
		powerhook_disestablish(sc->sc.sc_powerhook);
		sc->sc.sc_powerhook = NULL;
	}
#endif

	if (sc->sc_ih) {
		pxa2x0_intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	pxaohci_disable(sc);

	/* stop clock */
	pxa2x0_clkman_config(CKEN_USBHC, 0);

	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}

	return 0;
}

#if 0
static void
pxaohci_power(int why, void *arg)
{
	struct pxaohci_softc *sc = (struct pxaohci_softc *)arg;
	int s;

	s = splhardusb();
	sc->sc.sc_bus.use_polling++;
	switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
#if 0
		ohci_power(why, &sc->sc);
#endif
		pxa2x0_clkman_config(CKEN_USBHC, 0);
		break;

	case PWR_RESUME:
		pxa2x0_clkman_config(CKEN_USBHC, 1);
		pxaohci_enable(sc);
#if 0
		ohci_power(why, &sc->sc);
#endif
		break;
	}
	sc->sc.sc_bus.use_polling--;
	splx(s);
}
#endif

static void
pxaohci_enable(struct pxaohci_softc *sc)
{
	uint32_t hr;

	/* Full host reset */
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) | USBHC_HR_FHR);

	DELAY(USBHC_RST_WAIT);

	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) & ~(USBHC_HR_FHR));

	/* Force system bus interface reset */
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) | USBHC_HR_FSBIR);

	while (HREAD4(sc, USBHC_HR) & USBHC_HR_FSBIR)
		DELAY(3);

	/* Enable the ports (physically only one, only enable that one?) */
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) & ~(USBHC_HR_SSE));
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) &
			~(USBHC_HR_SSEP1 | USBHC_HR_SSEP2 | USBHC_HR_SSEP3));
	HWRITE4(sc, USBHC_HIE, USBHC_HIE_RWIE | USBHC_HIE_UPRIE);

	hr = HREAD4(sc, USBHC_UHCRHDA);
}

static void
pxaohci_disable(struct pxaohci_softc *sc)
{
	uint32_t hr;

	/* Full host reset */
	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) | USBHC_HR_FHR);

	DELAY(USBHC_RST_WAIT);

	hr = HREAD4(sc, USBHC_HR);
	HWRITE4(sc, USBHC_HR, (hr & USBHC_HR_MASK) & ~(USBHC_HR_FHR));
}


CFATTACH_DECL2_NEW(pxaohci, sizeof(struct pxaohci_softc),
    pxaohci_match, pxaohci_attach, pxaohci_detach, ohci_activate, NULL,
    ohci_childdet);


