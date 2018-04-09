/*
 * Copyright (c) 2013 Sughosh Ganu
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#include "opt_omapl1x.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: omapl1x_ohci.c,v 1.3 2018/04/09 16:21:09 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <arm/omap/omap_tipb.h>
#include <arm/omap/omapl1x_reg.h>
#include <arm/omap/omapl1x_misc.h>

struct omapl1xchipfg_softc {
	void		*sc_ih;
	bus_addr_t	sc_addr;
	bus_addr_t	sc_size;
};

struct omapl1xohci_softc {
	ohci_softc_t	sc;
	void		*sc_ih;
	int		sc_intr;
};

struct omapl1x_syscfg syscfg0;

static int omapl1xohci_match(struct device *, struct cfdata *, void *);
static void omapl1xohci_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(omapl1xohci, sizeof(struct omapl1xohci_softc),
    omapl1xohci_match, omapl1xohci_attach, NULL, NULL);

static int
omapl1xohci_match (struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;	/* XXX */
}

static void
omapl1xohci_attach (struct device *parent, struct device *self, void *aux)
{
	struct omapl1xohci_softc *sc = device_private(self);
	struct tipb_attach_args *tipb = aux;

	/* Map OHCI registers */
	if (bus_space_map(tipb->tipb_iot, tipb->tipb_addr, tipb->tipb_size, 0,
			  &sc->sc.ioh)) {
		aprint_error_dev(self, "can't map ohci mem space\n");
		return;
	}

	/* Enable the usb lpsc modules */
	if (omapl1x_lpsc_enable(PSC1_USB20_MODULE) != 0) {
		aprint_error_dev(self, "can't enable usb2.0 module\n");
		return;
	}

	if (omapl1x_lpsc_enable(PSC1_USB11_MODULE) != 0) {
		aprint_error_dev(self, "can't enable usb1.1 module\n");
		return;
	}

	syscfg0.syscfg_iot = tipb->tipb_iot;
	syscfg0.syscfg_addr = OMAPL1X_SYSCFG0_ADDR;
	syscfg0.syscfg_size = OMAPL1X_SYSCFG0_SIZE;

	/* Map syscfg registers. We want to use it for configuring cfgchip2 */
	if (bus_space_map(syscfg0.syscfg_iot, syscfg0.syscfg_addr,
			  syscfg0.syscfg_size, 0, &syscfg0.syscfg_ioh)) {
		aprint_error_dev(self, "can't map syscfg0 mem space\n");
		return;
	}

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_hcpriv = sc;
	sc->sc_intr = tipb->tipb_intr;
	sc->sc.iot = tipb->tipb_iot;
	sc->sc.sc_size = tipb->tipb_size;
	sc->sc.sc_bus.ub_dmatag = tipb->tipb_dmac;

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	/* provide clock to USB block */
	if (!omapl1x_usbclk_enable(&syscfg0)) {
		aprint_error_dev(self, "phy init failed\n");
		return;
	}

	/* establish the interrupt. */
	sc->sc_ih = intr_establish(sc->sc_intr, IPL_USB, IST_LEVEL_HIGH,
				   ohci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	int err = ohci_init(&sc->sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);

	/* Done accessing the syscfg0 registers */
	bus_space_unmap(syscfg0.syscfg_iot, syscfg0.syscfg_ioh,
			syscfg0.syscfg_size);
}
