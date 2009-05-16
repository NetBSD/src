/*	$NetBSD: ehci_cardbus.c,v 1.20.4.3 2009/05/16 10:41:19 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ehci_cardbus.c,v 1.20.4.3 2009/05/16 10:41:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/bus.h>

#if defined pciinc
#include <dev/pci/pcidevs.h>
#endif

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/usb_cardbus.h>

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

int	ehci_cardbus_match(device_t, cfdata_t, void *);
void	ehci_cardbus_attach(device_t, device_t, void *);
int	ehci_cardbus_detach(device_t, int);

struct ehci_cardbus_softc {
	ehci_softc_t		sc;
	cardbus_chipset_tag_t	sc_cc;
	cardbus_function_tag_t	sc_cf;
	cardbus_devfunc_t	sc_ct;
	void 			*sc_ih;		/* interrupt vectoring */
};

CFATTACH_DECL_NEW(ehci_cardbus, sizeof(struct ehci_cardbus_softc),
    ehci_cardbus_match, ehci_cardbus_attach, ehci_cardbus_detach, ehci_activate);

#define CARDBUS_INTERFACE_EHCI PCI_INTERFACE_EHCI
#define CARDBUS_CBMEM PCI_CBMEM
#define cardbus_findvendor pci_findvendor
#define cardbus_devinfo pci_devinfo

static TAILQ_HEAD(, usb_cardbus) ehci_cardbus_alldevs =
	TAILQ_HEAD_INITIALIZER(ehci_cardbus_alldevs);

int
ehci_cardbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct cardbus_attach_args *ca = (struct cardbus_attach_args *)aux;

	if (CARDBUS_CLASS(ca->ca_class) == CARDBUS_CLASS_SERIALBUS &&
	    CARDBUS_SUBCLASS(ca->ca_class) == CARDBUS_SUBCLASS_SERIALBUS_USB &&
	    CARDBUS_INTERFACE(ca->ca_class) == CARDBUS_INTERFACE_EHCI)
		return (1);

	return (0);
}

static bool
ehci_cardbus_suspend(device_t dv PMF_FN_ARGS)
{
	ehci_suspend(dv PMF_FN_CALL);
#if 0
	struct ehci_cardbus_softc *sc = device_private(dv);
	ehci_release_ownership(&sc->sc, sc->sc_pc, sc->sc_tag);
#endif

	return true;
}

static bool
ehci_cardbus_resume(device_t dv PMF_FN_ARGS)
{
#if 0
	struct ehci_cardbus_softc *sc = device_private(dv);
	ehci_get_ownership(&sc->sc, sc->sc_pc, sc->sc_tag);
#endif
	return ehci_resume(dv PMF_FN_CALL);
}

void
ehci_cardbus_attach(device_t parent, device_t self, void *aux)
{
	struct ehci_cardbus_softc *sc = device_private(self);
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbusreg_t csr;
	char devinfo[256];
	usbd_status r;
	const char *vendor;
	u_int ncomp;
	const char *devname = device_xname(self);
	struct usb_cardbus *up;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = sc;

	cardbus_devinfo(ca->ca_id, ca->ca_class, 0, devinfo, sizeof(devinfo));
	printf(": %s (rev. 0x%02x)\n", devinfo,
	       CARDBUS_REVISION(ca->ca_class));

	/* Map I/O registers */
	if (Cardbus_mapreg_map(ct, CARDBUS_CBMEM, CARDBUS_MAPREG_TYPE_MEM, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc.sc_size)) {
		printf("%s: can't map mem space\n", devname);
		return;
	}

	sc->sc_cc = cc;
	sc->sc_cf = cf;
	sc->sc_ct = ct;
	sc->sc.sc_bus.dmatag = ca->ca_dmat;

#if rbus
#else
XXX	(ct->ct_cf->cardbus_mem_open)(cc, 0, iob, iob + 0x40);
#endif
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_MEM_ENABLE);
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Enable the device. */
	csr = cardbus_conf_read(cc, cf, ca->ca_tag,
				CARDBUS_COMMAND_STATUS_REG);
	cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG,
		       csr | CARDBUS_COMMAND_MASTER_ENABLE
			   | CARDBUS_COMMAND_MEM_ENABLE);

	/* Disable interrupts, so we don't get any spurious ones. */
	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);
	DPRINTF(("%s: offs=%d\n", devname, sc->sc.sc_offs));
	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);

	sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline,
					   IPL_USB, ehci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n", devname);
		return;
	}

	/* Figure out vendor for root hub descriptor. */
	vendor = cardbus_findvendor(ca->ca_id);
	sc->sc.sc_id_vendor = CARDBUS_VENDOR(ca->ca_id);
	if (vendor)
		strlcpy(sc->sc.sc_vendor, vendor, sizeof(sc->sc.sc_vendor));
	else
		snprintf(sc->sc.sc_vendor, sizeof(sc->sc.sc_vendor),
		    "vendor 0x%04x", CARDBUS_VENDOR(ca->ca_id));

	/*
	 * Find companion controllers.  According to the spec they always
	 * have lower function numbers so they should be enumerated already.
	 */
	ncomp = 0;
	TAILQ_FOREACH(up, &ehci_cardbus_alldevs, next) {
		if (up->bus == ca->ca_bus) {
			DPRINTF(("ehci_cardbus_attach: companion %s\n",
				 device_xname(up->usb)));
			sc->sc.sc_comps[ncomp++] = up->usb;
			if (ncomp >= EHCI_COMPANION_MAX)
				break;
		}
	}
	sc->sc.sc_ncomp = ncomp;

	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);

		/* Avoid spurious interrupts. */
		cardbus_intr_disestablish(sc->sc_cc, sc->sc_cf, sc->sc_ih);
		sc->sc_ih = NULL;

		return;
	}

	if (!pmf_device_register1(self, ehci_cardbus_suspend,
	                          ehci_cardbus_resume, ehci_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Attach usb device. */
	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

int
ehci_cardbus_detach(device_t self, int flags)
{
	struct ehci_cardbus_softc *sc = device_private(self);
	struct cardbus_devfunc *ct = sc->sc_ct;
	int rv;

	rv = ehci_detach(&sc->sc, flags);
	if (rv)
		return (rv);
	if (sc->sc_ih != NULL) {
		cardbus_intr_disestablish(sc->sc_cc, sc->sc_cf, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc.sc_size) {
		Cardbus_mapreg_unmap(ct, CARDBUS_CBMEM, sc->sc.iot,
		    sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}
	return (0);
}

void
usb_cardbus_add(struct usb_cardbus *up, struct cardbus_attach_args *ca, device_t bu)
{
	TAILQ_INSERT_TAIL(&ehci_cardbus_alldevs, up, next);
	up->bus = ca->ca_bus;
	up->function = ca->ca_function;
	up->usb = bu;
}

void
usb_cardbus_rem(struct usb_cardbus *up)
{
	TAILQ_REMOVE(&ehci_cardbus_alldevs, up, next);
}
