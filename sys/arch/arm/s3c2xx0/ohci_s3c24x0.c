/*	$NetBSD: ohci_s3c24x0.c,v 1.1 2003/08/05 11:28:59 bsh Exp $ */

/* derived from ohci_pci.c */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: ohci_s3c24x0.c,v 1.1 2003/08/05 11:28:59 bsh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <arm/s3c2xx0/s3c24x0var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

int	ohci_ssio_match(struct device *, struct cfdata *, void *);
void	ohci_ssio_attach(struct device *, struct device *, void *);
int	ohci_ssio_detach(device_ptr_t, int);

extern	int ohcidebug;

struct ohci_ssio_softc {
	ohci_softc_t		sc;

	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL(ohci_ssio, sizeof(struct ohci_ssio_softc),
    ohci_ssio_match, ohci_ssio_attach, ohci_ssio_detach, ohci_activate);

int
ohci_ssio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;
	/* XXX: check some registers */

	if (sa->sa_dmat == NULL) {
		/* busdma tag is not initialized. */
		return 0;
	}

	return 1;
}

void
ohci_ssio_attach(struct device *parent, struct device *self, void *aux)
{
	struct ohci_ssio_softc *sc = (struct ohci_ssio_softc *)self;
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args *)aux;

	usbd_status r;
	char *devname = sc->sc.sc_bus.bdev.dv_xname;

	sc->sc.iot = sa->sa_iot;
	/*ohcidebug=15;*/

	/* Map I/O registers */
	if (bus_space_map(sc->sc.iot, sa->sa_addr, 0x5c, 0, &sc->sc.ioh)) {
		printf("%s: can't map mem space\n", devname);
		return;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	printf("dmat = %p\n", sa->sa_dmat);
	sc->sc.sc_bus.dmatag = sa->sa_dmat;

	/* Enable the device. */
	/* XXX: provide clock to USB block */

	/* establish the interrupt. */
	sc->sc_ih = s3c24x0_intr_establish(sa->sa_intr, IPL_USB, IST_NONE, ohci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n", devname);
		return;
	}

	/*strlcpy(sc->sc.sc_vendor, "Samsung", sizeof sc->sc.sc_id_vendor);*/
	
	r = ohci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

int
ohci_ssio_detach(device_ptr_t self, int flags)
{
	return (0);
}
