/*	$NetBSD: uhci_pci.c,v 1.36.2.1 2007/08/03 22:17:22 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uhci_pci.c,v 1.36.2.1 2007/08/03 22:17:22 jmcneill Exp $");

#include "ehci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/usb_pci.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/uhcireg.h>
#include <dev/usb/uhcivar.h>

static pnp_status_t	uhci_pci_power(device_t, pnp_request_t, void *);

struct uhci_pci_softc {
	uhci_softc_t		sc;
#if NEHCI > 0
	struct usb_pci		sc_pci;
#endif
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void 			*sc_ih;		/* interrupt vectoring */

	struct pci_conf_state	sc_pciconf;
};

static int
uhci_pci_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_UHCI)
		return (1);

	return (0);
}

static void
uhci_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct uhci_pci_softc *sc = (struct uhci_pci_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pnp_status_t status;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	const char *vendor;
	const char *devname = sc->sc.sc_bus.bdev.dv_xname;
	char devinfo[256];
	usbd_status r;

	aprint_naive("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n",
		      devinfo, PCI_REVISION(pa->pa_class));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc.sc_size)) {
		aprint_error("%s: can't map i/o space\n", devname);
		return;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_2(sc->sc.iot, sc->sc.ioh, UHCI_INTR, 0);

	sc->sc_pc = pc;
	sc->sc_tag = tag;
	sc->sc.sc_bus.dmatag = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: couldn't map interrupt\n", devname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_USB, uhci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt", devname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", devname, intrstr);

	/* Set LEGSUP register to its default value. */
	pci_conf_write(pc, tag, PCI_LEGSUP, PCI_LEGSUP_USBPIRQDEN);

	switch(pci_conf_read(pc, tag, PCI_USBREV) & PCI_USBREV_MASK) {
	case PCI_USBREV_PRE_1_0:
		sc->sc.sc_bus.usbrev = USBREV_PRE_1_0;
		break;
	case PCI_USBREV_1_0:
		sc->sc.sc_bus.usbrev = USBREV_1_0;
		break;
	case PCI_USBREV_1_1:
		sc->sc.sc_bus.usbrev = USBREV_1_1;
		break;
	default:
		sc->sc.sc_bus.usbrev = USBREV_UNKNOWN;
		break;
	}

	/* Figure out vendor for root hub descriptor. */
	vendor = pci_findvendor(pa->pa_id);
	sc->sc.sc_id_vendor = PCI_VENDOR(pa->pa_id);
	if (vendor)
		strlcpy(sc->sc.sc_vendor, vendor, sizeof(sc->sc.sc_vendor));
	else
		snprintf(sc->sc.sc_vendor, sizeof(sc->sc.sc_vendor),
		    "vendor 0x%04x", PCI_VENDOR(pa->pa_id));

	r = uhci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error("%s: init failed, error=%d\n", devname, r);
		return;
	}

#if NEHCI > 0
	usb_pci_add(&sc->sc_pci, pa, &sc->sc.sc_bus);
#endif

	status = pnp_register(self, uhci_pci_power);
	if (status != PNP_STATUS_SUCCESS)
		aprint_error("%s: couldn't establish power handler\n",
		    devname);

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

static int
uhci_pci_detach(device_ptr_t self, int flags)
{
	struct uhci_pci_softc *sc = (struct uhci_pci_softc *)self;
	int rv;

	rv = uhci_detach(&sc->sc, flags);
	if (rv)
		return (rv);
	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}
#if NEHCI > 0
	usb_pci_rem(&sc->sc_pci);
#endif
	return (0);
}

static pnp_status_t
uhci_pci_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct uhci_pci_softc *sc;
	pnp_status_t status;
	pnp_capabilities_t *pcaps;
	pnp_state_t *pstate;
	pci_chipset_tag_t pc;
	pcitag_t tag;
	pcireg_t reg;
	int off, s;

	sc = (struct uhci_pci_softc *)dv;
	pc = sc->sc_pc;
	tag = sc->sc_tag;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		pcaps = opaque;

		pci_get_capability(pc, tag, PCI_CAP_PWRMGMT, &off, &reg);
		pcaps->state = pci_pnp_capabilities(reg);
		pcaps->state |= PNP_STATE_D0 | PNP_STATE_D3;
		status = uhci_power(dv, req, opaque);
		break;

	case PNP_REQUEST_GET_STATE:
		pstate = opaque;
		if (pci_get_powerstate(pc, tag, &reg) != 0)
			*pstate = PNP_STATE_D0;
		else
			*pstate = pci_pnp_powerstate(reg);
		status = PNP_STATUS_SUCCESS;
		break;

	case PNP_REQUEST_SET_STATE:
		pstate = opaque;
		switch (*pstate) {
		case PNP_STATE_D0:
			reg = PCI_PMCSR_STATE_D0;
			break;
		case PNP_STATE_D3:
			reg = PCI_PMCSR_STATE_D3;
			s = splhardusb();
			pci_conf_capture(pc, tag, &sc->sc_pciconf);
			splx(s);
			status = uhci_power(dv, req, opaque);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}

		status = PNP_STATUS_SUCCESS;
		(void)pci_set_powerstate(pc, tag, reg);
		if (*pstate != PNP_STATE_D0)
			break;

		s = splhardusb();
		pci_conf_restore(pc, tag, &sc->sc_pciconf);
		/* the BIOS might change this on us */
		reg = pci_conf_read(pc, tag, PCI_LEGSUP);
		reg |= PCI_LEGSUP_USBPIRQDEN;
		reg &= ~PCI_LEGSUP_USBSMIEN;
		pci_conf_write(pc, tag, PCI_LEGSUP, reg);
		splx(s);

		status = uhci_power(dv, req, opaque);
		break;

	default:
		status = PNP_STATUS_UNSUPPORTED;
		break;
	}

	return status;
}

CFATTACH_DECL(uhci_pci, sizeof(struct uhci_pci_softc),
    uhci_pci_match, uhci_pci_attach, uhci_pci_detach, uhci_activate);
