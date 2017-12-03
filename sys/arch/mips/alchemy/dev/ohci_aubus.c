/*	$NetBSD: ohci_aubus.c,v 1.15.12.1 2017/12/03 11:36:26 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Herb Peyerl of Middle Digital Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ohci_aubus.c,v 1.15.12.1 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/dev/ohcireg.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>


static int	ohci_aubus_match(device_t, cfdata_t, void *);
static void	ohci_aubus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_aubus, sizeof (ohci_softc_t),
    ohci_aubus_match, ohci_aubus_attach, NULL, NULL);

int
ohci_aubus_match(device_t parent, cfdata_t match, void *aux)
{
	struct aubus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, match->cf_name) == 0)
		return 1;

	return 0;
}

void
ohci_aubus_attach(device_t parent, device_t self, void *aux)
{
	ohci_softc_t *sc = device_private(self);
	void *ih;
	uint32_t x, tmp;
	bus_addr_t usbh_base, usbh_enable;
	struct aubus_attach_args *aa = aux;

	usbh_base = aa->aa_addrs[0];
	usbh_enable = aa->aa_addrs[1];
	sc->sc_size = aa->aa_addrs[2];
	sc->iot = aa->aa_st;
	sc->sc_bus.ub_dmatag = (bus_dma_tag_t)aa->aa_dt;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;

	if (bus_space_map(sc->iot, usbh_base, sc->sc_size, 0, &sc->ioh)) {
		aprint_error_dev(self, "unable to map USBH registers\n");
		return;
	}
	/*
	 * Enable the USB Host controller here.
	 * As per 7.2 in the Au1500 manual:
	 *
	 *  (1) Set CE bit to enable clocks.
	 *  (2) Set E to enable OHCI
	 *  (3) Clear HCFS in OHCI_CONTROL.
	 *  (4) Wait for RD bit to be set.
	 */
	x = bus_space_read_4(sc->iot, sc->ioh, usbh_enable);
	x |= UE_CE;
	bus_space_write_4(sc->iot, sc->ioh, usbh_enable, x);
	delay(10);
	x |= UE_E;
#ifdef __MIPSEB__
	x |= UE_BE;
#endif
	bus_space_write_4(sc->iot, sc->ioh, usbh_enable, x);
	delay(10);
	x = bus_space_read_4(sc->iot, sc->ioh, OHCI_CONTROL);
	x &= ~(OHCI_HCFS_MASK);
	bus_space_write_4(sc->iot, sc->ioh, OHCI_CONTROL, x);
	delay(10);
	/*  Need to read USBH_ENABLE twice in succession according to
         *  au1500 Errata #7.
         */
	for (x = 100; x; x--) {
		bus_space_read_4(sc->iot, sc->ioh, usbh_enable);
		tmp = bus_space_read_4(sc->iot, sc->ioh, usbh_enable);
		if (tmp&UE_RD)
			break;
		delay(1000);
	}

	if (x == 0) {
		aprint_error_dev(self, "device not ready\n");
		return;
	}

	printf(": Alchemy OHCI\n");

	/* Disable OHCI interrupts */
	bus_space_write_4(sc->iot, sc->ioh, OHCI_INTERRUPT_DISABLE,
				OHCI_ALL_INTRS);
	/* hook interrupt */
	ih = au_intr_establish(aa->aa_irq[0], 0, IPL_USB, IST_LEVEL_LOW,
			ohci_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self,"couldn't establish interrupt\n");
	}

	sc->sc_endian = OHCI_HOST_ENDIAN;

	int err = ohci_init(sc);
	if (err != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		au_intr_disestablish(ih);
		return;
	}

	/* Attach USB device */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);

}
