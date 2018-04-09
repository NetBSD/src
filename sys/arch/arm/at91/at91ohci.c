/*	$Id: at91ohci.c,v 1.7 2018/04/09 16:21:09 jakllsch Exp $	*/
/*	$NetBSD: at91ohci.c,v 1.7 2018/04/09 16:21:09 jakllsch Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy.
 * All rights reserved.
 *
 * Based on arch/arm/ep93xx/epohci.c,
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
__KERNEL_RCSID(0, "$NetBSD: at91ohci.c,v 1.7 2018/04/09 16:21:09 jakllsch Exp $");

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

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>

int at91ohci_match(device_t, cfdata_t, void *);
void at91ohci_attach(device_t, device_t, void *);
void at91ohci_callback(device_t );

struct at91ohci_softc {
	struct ohci_softc sc;
	void	*sc_ih;
	int	sc_pid;
};

CFATTACH_DECL_NEW(at91ohci, sizeof(struct at91ohci_softc),
    at91ohci_match, at91ohci_attach, NULL, NULL);

int
at91ohci_match(device_t parent, cfdata_t match, void *aux)
{
	/* AT91X builtin OHCI module */
	if (strcmp(match->cf_name, "ohci") == 0 && strcmp(match->cf_atname, "at91ohci") == 0)
		return 2;
	return(0);
}

void
at91ohci_attach(device_t parent, device_t self, void *aux)
{
	struct at91ohci_softc *sc = device_private(self);
	struct at91bus_attach_args *sa = aux;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_hcpriv = sc;
	sc->sc.sc_bus.ub_dmatag = sa->sa_dmat;
	sc->sc.iot = sa->sa_iot;
	sc->sc_pid = sa->sa_pid;

	/* Map I/O space */
	if (bus_space_map(sc->sc.iot, sa->sa_addr, sa->sa_size,
			  0, &sc->sc.ioh)) {
		printf(": cannot map mem space\n");
		return;
	}

	sc->sc.sc_size = sa->sa_size;

	/* enable peripheral clock */
	at91_peripheral_clock(sc->sc_pid, 1);

	printf("\n");

        /* Defer the rest until later */
        config_defer(self, at91ohci_callback);
}

void
at91ohci_callback(device_t self)
{
	struct at91ohci_softc *sc = device_private(self);

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	sc->sc_ih = at91_intr_establish(sc->sc_pid, IPL_USB, INTR_HIGH_LEVEL, ohci_intr, sc);
	int err = ohci_init(&sc->sc);

	if (err) {
		printf("%s: init failed, error=%d\n", device_xname(self), err);

		at91_intr_disestablish(sc->sc_ih);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}
