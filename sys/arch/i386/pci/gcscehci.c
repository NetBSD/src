/* $NetBSD: gcscehci.c,v 1.4.10.2 2009/11/01 13:58:35 jym Exp $ */

/*
 * Copyright (c) 2001, 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net)
 * and Jared D. McNeill (jmcneill@invisible.ca)
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
__KERNEL_RCSID(0, "$NetBSD: gcscehci.c,v 1.4.10.2 2009/11/01 13:58:35 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/usb_pci.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#ifdef EHCI_DEBUG
#define DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define DPRINTF(x)
#endif

#define GCSCUSB_MSR_BASE	0x51200000
#define GCSCUSB_MSR_EHCB	(GCSCUSB_MSR_BASE + 0x09)

struct gcscehci_softc {
	ehci_softc_t		sc;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void 			*sc_ih;		/* interrupt vectoring */
};

static int
gcscehci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_EHCI &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_CS5536_EHCI)
		return (10);	/* beat ehci_pci */

	return (0);
}

static void
gcscehci_attach(device_t parent, device_t self, void *aux)
{
	struct gcscehci_softc *sc = device_private(self);
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	char const *intrstr;
	pci_intr_handle_t ih;
	const char *vendor;
	const char *devname = device_xname(self);
	char devinfo[256];
	usbd_status r;
	bus_addr_t ehcibase;
	int ncomp;
	struct usb_pci *up;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = sc;

	aprint_naive(": USB controller\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	/* Map I/O registers */
	ehcibase = rdmsr(GCSCUSB_MSR_EHCB) & 0xffffff00;
	sc->sc.iot = pa->pa_memt;
	sc->sc.sc_size = 256;
	if (bus_space_map(sc->sc.iot, ehcibase, 256, 0, &sc->sc.ioh)) {
		aprint_error("%s: can't map memory space\n", devname);
		return;
	}

	sc->sc_pc = pc;
	sc->sc_tag = tag;
	sc->sc.sc_bus.dmatag = pa->pa_dmat;

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);
	DPRINTF(("%s: offs=%d\n", devname, sc->sc.sc_offs));
	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: couldn't map interrupt\n", devname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_USB, ehci_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt", devname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", devname, intrstr);

	sc->sc.sc_bus.usbrev = USBREV_2_0;

	/* Figure out vendor for root hub descriptor. */
	vendor = pci_findvendor(pa->pa_id);
	sc->sc.sc_id_vendor = PCI_VENDOR(pa->pa_id);
	if (vendor)
		strlcpy(sc->sc.sc_vendor, vendor, sizeof(sc->sc.sc_vendor));
	else
		snprintf(sc->sc.sc_vendor, sizeof(sc->sc.sc_vendor),
		    "vendor 0x%04x", PCI_VENDOR(pa->pa_id));

	/*
	 * Find companion controllers.  According to the spec they always
	 * have lower function numbers so they should be enumerated already.
	 */
	ncomp = 0;
	TAILQ_FOREACH(up, &ehci_pci_alldevs, next) {
		if (up->bus == pa->pa_bus && up->device == pa->pa_device) {
			DPRINTF(("gcscehci_attach: companion %s\n",
				 device_xname(up->usb)));
			sc->sc.sc_comps[ncomp++] = up->usb;
			if (ncomp >= EHCI_COMPANION_MAX)
				break;
		}
	}
	sc->sc.sc_ncomp = ncomp;

	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error("%s: init failed, error=%d\n", devname, r);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

CFATTACH_DECL_NEW(gcscehci, sizeof(struct gcscehci_softc),
    gcscehci_match, gcscehci_attach, NULL, ehci_activate);
