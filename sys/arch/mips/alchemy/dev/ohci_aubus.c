/*	$NetBSD: ohci_aubus.c,v 1.3 2003/07/15 02:43:35 lukem Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ohci_aubus.c,v 1.3 2003/07/15 02:43:35 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>

#include <evbmips/alchemy/pb1000reg.h>
#include <evbmips/alchemy/pb1000_obiovar.h>

#include <dev/usb/usb.h>   
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>


static int	ohci_aubus_match(struct device *, struct cfdata *, void *);
static void	ohci_aubus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ohci_aubus, sizeof (ohci_softc_t),
    ohci_aubus_match, ohci_aubus_attach, NULL, NULL);

struct aubus_ohci_softc {
	ohci_softc_t sc;
	void *sc_ih;
};

int
ohci_aubus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
ohci_aubus_attach(struct device *parent, struct device *self, void *aux)
{
	struct aubus_ohci_softc *sc = (struct aubus_ohci_softc *)self;
	usbd_status r;
	uint32_t x, tmp;
	struct aubus_attach_args *aa = aux;

	r = 0;

	sc->sc.sc_size = USBH_SIZE;
	sc->sc.iot = aa->aa_st;
	sc->sc.sc_bus.dmatag = (bus_dma_tag_t)aa->aa_dt;

	if (bus_space_map(sc->sc.iot, USBH_BASE, USBH_SIZE, 0, &sc->sc.ioh)) {
		printf("%s: Unable to map USBH registers\n",
			sc->sc.sc_bus.bdev.dv_xname);
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
	x = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBH_ENABLE);
	x |= UE_CE;
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBH_ENABLE, x);
	delay(10);
	x |= (UE_E);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, USBH_ENABLE, x);
	delay(10);
	x = bus_space_read_4(sc->sc.iot, sc->sc.ioh, OHCI_CONTROL);
	x &= ~(OHCI_HCFS_MASK);
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_CONTROL, x);
	delay(10);
	/*  Need to read USBH_ENABLE twice in succession according to
         *  au1500 Errata #7.
         */
	for (x = 100; x; x--) {
		bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBH_ENABLE);
		tmp = bus_space_read_4(sc->sc.iot, sc->sc.ioh, USBH_ENABLE);
		if (tmp&UE_RD)
			break;
		delay(1000);
	}
	printf(": Au1X00 OHCI\n");

	/* Disable OHCI interrupts */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
				OHCI_ALL_INTRS);
	/* hook interrupt */
	sc->sc_ih = au_intr_establish(aa->aa_irq[0], 0, IPL_USB, IST_LEVEL_LOW,
			ohci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
			sc->sc.sc_bus.bdev.dv_xname);
	}

	if (x)
		r = ohci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n",
			sc->sc.sc_bus.bdev.dv_xname, r);
		return;
	}

	/* Attach USB device */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
					usbctlprint);

}
