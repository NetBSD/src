/*	$Id: obio_ohci.c,v 1.5.2.1 2012/04/17 00:06:06 yamt Exp $	*/

/* adapted from: */
/*	$NetBSD: obio_ohci.c,v 1.5.2.1 2012/04/17 00:06:06 yamt Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_ohci.c,v 1.5.2.1 2012/04/17 00:06:06 yamt Exp $");

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
	void		*sc_powerhook;
};

static int	obioohci_match(struct device *, struct cfdata *, void *);
static void	obioohci_attach(struct device *, struct device *, void *);
static int	obioohci_detach(struct device *, int);
void *		obioohci_fake_intr_establish(int (*)(void *), void *);
void		obioohci_fake_intr(void);

CFATTACH_DECL_NEW(obioohci, sizeof(struct obioohci_softc),
    obioohci_match, obioohci_attach, obioohci_detach, ohci_activate);

static void	obioohci_clkinit(struct obio_attach_args *);
static void	obioohci_power(int, void *);
static void	obioohci_enable(struct obioohci_softc *);
static void	obioohci_disable(struct obioohci_softc *);

#define	HREAD4(sc,r)	bus_space_read_4((sc)->sc.iot, (sc)->sc.ioh, (r))
#define	HWRITE4(sc,r,v)	bus_space_write_4((sc)->sc.iot, (sc)->sc.ioh, (r), (v))

static int
obioohci_match(device_t parent, cfdata_t cf, void *aux)
{

	struct obio_attach_args *obio = aux;

	if ((obio->obio_addr == -1)
	|| (obio->obio_size == 0)
	|| (obio->obio_intr == -1))
		panic("obioohci must have addr, size and intr"
			" specified in config.");

	obioohci_clkinit(obio);

	return 1;	/* XXX */
}

static void
obioohci_attach(device_t parent, device_t self, void *aux)
{
	struct obioohci_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	usbd_status r;

	sc->sc.sc_size = 0;
	sc->sc_ih = NULL;
	sc->sc.sc_bus.dmatag = 0;

	/* Map I/O space */
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
	    &sc->sc.ioh)) {
		aprint_error(": couldn't map memory space\n");
		return;
	}
	sc->sc.iot = obio->obio_iot;
	sc->sc_addr = obio->obio_addr;
	sc->sc.sc_size = obio->obio_size;
	sc->sc.sc_bus.dmatag = obio->obio_dmat;

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
	    OHCI_MIE);

#if 1
	sc->sc_ih = intr_establish(obio->obio_intr, IPL_USB, IST_LEVEL,
		ohci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		goto free_map;
	}
#else
	sc->sc_ih = obioohci_fake_intr_establish(ohci_intr, &sc->sc);
#endif

	strlcpy(sc->sc.sc_vendor, "OMAP2", sizeof(sc->sc.sc_vendor));
	r = ohci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error=%d\n", r);
		goto free_intr;
	}

	sc->sc_powerhook = powerhook_establish(device_xname(self),
	    obioohci_power, sc);
	if (sc->sc_powerhook == NULL) {
		aprint_error_dev(self, "cannot establish powerhook\n");
	}

	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus, usbctlprint);

	return;

free_intr:
#ifdef NOTYET
	obio_gpio_intr_disestablish(sc->sc_ih);
#endif
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

	if (sc->sc_powerhook) {
		powerhook_disestablish(sc->sc_powerhook);
		sc->sc_powerhook = NULL;
	}

	if (sc->sc_ih) {
#ifdef NOTYET
		obio_gpio_intr_disestablish(sc->sc_ih);
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
obioohci_power(int why, void *arg)
{
	struct obioohci_softc *sc = (struct obioohci_softc *)arg;
	int s;

	s = splhardusb();
	sc->sc.sc_bus.use_polling++;
	switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
#if 0
		ohci_power(why, &sc->sc);
#endif
#ifdef NOTYET
		pxa2x0_clkman_config(CKEN_USBHC, 0);
#endif
		break;

	case PWR_RESUME:
#ifdef NOTYET
		pxa2x0_clkman_config(CKEN_USBHC, 1);
#endif
		obioohci_enable(sc);
#if 0
		ohci_power(why, &sc->sc);
#endif
		break;
	}
	sc->sc.sc_bus.use_polling--;
	splx(s);
}

static void
obioohci_enable(struct obioohci_softc *sc)
{
	printf("%s: TBD\n", __func__);
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
}

#if 0
int (*obioohci_fake_intr_func)(void *);
void *obioohci_fake_intr_arg;

void *
obioohci_fake_intr_establish(int (*func)(void *), void *arg)
{
	obioohci_fake_intr_func = func;
	obioohci_fake_intr_arg = arg;
	return (void *)1;
}

void
obioohci_fake_intr(void)
{
	(void)(*obioohci_fake_intr_func)(obioohci_fake_intr_arg);
}
#endif
