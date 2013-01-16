/*	$NetBSD: epohci.c,v 1.5.2.2 2013/01/16 05:32:46 yamt Exp $ */

/*-
 * Copyright (c) 2004 Jesse Off
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * USB Open Host Controller driver.
 *
 * OHCI spec: ftp://ftp.compaq.com/pub/supportinformation/papers/hcir1_0a.exe
 * USB spec: http://www.usb.org/developers/data/usb11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: epohci.c,v 1.5.2.2 2013/01/16 05:32:46 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

/* busdma */
#include <sys/mbuf.h>
#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/ep93xxvar.h>
#include <arm/ep93xx/epsocvar.h>

int epohci_match(device_t, cfdata_t, void *);
void epohci_attach(device_t, device_t, void *);
void epohci_callback(device_t);

struct epohci_softc {
	struct ohci_softc sc;
	void	*sc_ih;
	int	sc_intr;
};

CFATTACH_DECL_NEW(epohci, sizeof(struct epohci_softc),
    epohci_match, epohci_attach, NULL, NULL);

int
epohci_match(device_t parent, cfdata_t match, void *aux)
{
	/* EP93xx builtin OHCI module */

	return (1);
}

void
epohci_attach(device_t parent, device_t self, void *aux)
{
	struct epohci_softc *sc = device_private(self);
	struct epsoc_attach_args *sa = aux;
	uint32_t i;
	bus_space_handle_t syscon_ioh;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = sc;

	sc->sc.iot = sa->sa_iot;
	sc->sc.sc_bus.dmatag = sa->sa_dmat;
	sc->sc_intr = sa->sa_intr;

	/* Map I/O space */
	if (bus_space_map(sc->sc.iot, sa->sa_addr, sa->sa_size, 
	    0, &sc->sc.ioh)) {
		printf(": cannot map mem space\n");
		return;
	}

	/* Enable hclk clock gating to the USB block. */

	bus_space_map(sc->sc.iot, EP93XX_APB_HWBASE + EP93XX_APB_SYSCON,
		EP93XX_APB_SYSCON_SIZE, 0, &syscon_ioh);
	i = bus_space_read_4(sc->sc.iot, syscon_ioh, EP93XX_SYSCON_PwrCnt);
	i |= 0x10000000;
	bus_space_write_4(sc->sc.iot, syscon_ioh, EP93XX_SYSCON_PwrCnt, i);

	/*
	 * Not sure if I understand the datasheet here, but we must wait a
	 * few instructions and also make certain PLL2 is stable before
	 * continuing.  Hopefully, the check for PLL2 is enough, as the
	 * datasheet is elusive to actually how many insns we need.
	 */

	do {
		i = bus_space_read_4(sc->sc.iot, syscon_ioh, 
			EP93XX_SYSCON_PwrSts);
	} while ((i & 0x100) == 0);
	bus_space_unmap(sc->sc.iot, syscon_ioh, EP93XX_APB_SYSCON_SIZE);

	printf("\n");

        /* Defer the rest until later */
        config_defer(self, epohci_callback);
}

void
epohci_callback(device_t self)
{
	struct epohci_softc *sc = device_private(self);
	usbd_status r;

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	strlcpy(sc->sc.sc_vendor, "Cirrus Logic", sizeof sc->sc.sc_vendor);

	sc->sc_ih = ep93xx_intr_establish(sc->sc_intr, IPL_USB, 
		ohci_intr, sc);
	r = ohci_init(&sc->sc);

	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", device_xname(self), r);

		ep93xx_intr_disestablish(sc->sc_ih);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);

}
