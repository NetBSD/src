/*	$NetBSD: mpc5200_ohci.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Robert Swindells.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * USB OHCI host controller on the Freescale MPC5200B SoC.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpc5200_ohci.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ofw/openfirm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/mpc5200_ohcivar.h>

static int	mpc5200_ohci_match(device_t, cfdata_t, void *);
static void	mpc5200_ohci_attach(device_t, device_t, void *);

CFATTACH_DECL2_NEW(ohci_obio, sizeof(struct mpc5200_ohci_softc),
    mpc5200_ohci_match, mpc5200_ohci_attach, NULL, ohci_activate,
    NULL, ohci_childdet);

static int
mpc5200_ohci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[40];
	int len;

	if (strcmp(oba->obio_name, "usb") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-ohci") == 0 ||
	     strcmp(compat, "mpc5200b-ohci") == 0 ||
	     strcmp(compat, "mpc5200-usb-ohci") == 0 ||
	     strcmp(compat, "mpc5200b-usb-ohci") == 0))
		return 1;

	return 0;
}

static void
mpc5200_ohci_attach(device_t parent, device_t self, void *aux)
{
	struct mpc5200_ohci_softc *msc = device_private(self);
	ohci_softc_t *sc = &msc->sc;
	struct obio_attach_args *oba = aux;
	bus_addr_t addr;
	bus_size_t size;
	int ist, error;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = oba->obio_dmat;
	sc->iot = oba->obio_bst;
	sc->sc_endian = OHCI_HOST_ENDIAN;

	/*
	 * Honour the firmware, but fall back to SoC MBAR offset.
	 */
	addr = oba->obio_addr != 0 ? oba->obio_addr :
	    MPC5200_MBAR_DEFAULT + MPC5200_REG_USB;
	size = oba->obio_size != 0 ? oba->obio_size : MPC5200_USB_SIZE;
	sc->sc_size = size;

	aprint_naive("\n");
	aprint_normal(": MPC5200 USB OHCI\n");

	if (bus_space_map(sc->iot, addr, size, 0, &sc->ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	/* Mask all controller interrupts until the MI driver is ready. */
	bus_space_write_4(sc->iot, sc->ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	if (!obio_decode_interrupt(oba->obio_node, 0, &msc->sc_irq, &ist)) {
		aprint_error_dev(self, "can't decode interrupt\n");
		goto fail_unmap;
	}
	msc->sc_ih = intr_establish(msc->sc_irq, ist, IPL_USB, ohci_intr, sc);
	if (msc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		goto fail_unmap;
	}

	error = ohci_init(sc);
	if (error != 0) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		goto fail_intr;
	}

	if (!pmf_device_register1(self, NULL, NULL, ohci_shutdown))
		aprint_error_dev(self, "can't establish power handler\n");

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint,
	    CFARGS_NONE);
	return;

fail_intr:
	intr_disestablish(msc->sc_ih);
	msc->sc_ih = NULL;
fail_unmap:
	bus_space_unmap(sc->iot, sc->ioh, size);
	sc->sc_size = 0;
}
