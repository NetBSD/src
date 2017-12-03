/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#define USBH_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_usb.c,v 1.1.2.4 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <dev/pci/pcidevs.h>

struct bcmusb_softc {
	device_t usbsc_dev;
	bus_dma_tag_t usbsc_dmat;
	bus_space_tag_t usbsc_bst;
	bus_space_handle_t usbsc_ehci_bsh;
	bus_space_handle_t usbsc_ohci_bsh;

	device_t usbsc_ohci_dev;
	device_t usbsc_ehci_dev;
	void *usbsc_ohci_sc;
	void *usbsc_ehci_sc;
	void *usbsc_ih;
};

struct bcmusb_attach_args {
	const char *usbaa_name;
	bus_dma_tag_t usbaa_dmat;
	bus_space_tag_t usbaa_bst;
	bus_space_handle_t usbaa_bsh;
	bus_size_t usbaa_size;
};

#ifdef OHCI_DEBUG
#define OHCI_DPRINTF(x)	if (ohcidebug) printf x
extern int ohcidebug;
#else
#define OHCI_DPRINTF(x)
#endif

static int ohci_bcmusb_match(device_t, cfdata_t, void *);
static void ohci_bcmusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_bcmusb, sizeof(struct ohci_softc),
	ohci_bcmusb_match, ohci_bcmusb_attach, NULL, NULL);

static int
ohci_bcmusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ohci_bcmusb_attach(device_t parent, device_t self, void *aux)
{
	struct ohci_softc * const sc = device_private(self);
	struct bcmusb_attach_args * const usbaa = aux;

	sc->sc_dev = self;

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.ub_dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.ub_hcpriv = sc;

	sc->sc_id_vendor = PCI_VENDOR_BROADCOM;
	strlcpy(sc->sc_vendor, "Broadcom", sizeof(sc->sc_vendor));

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller\n");

	int error = ohci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}

	/* Attach usb device. */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

#ifdef EHCI_DEBUG
#define EHCI_DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define EHCI_DPRINTF(x)
#endif

static int ehci_bcmusb_match(device_t, cfdata_t, void *);
static void ehci_bcmusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ehci_bcmusb, sizeof(struct ehci_softc),
	ehci_bcmusb_match, ehci_bcmusb_attach, NULL, NULL);

static int
ehci_bcmusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ehci_bcmusb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmusb_softc * const usbsc = device_private(parent);
	struct ehci_softc * const sc = device_private(self);
	struct bcmusb_attach_args * const usbaa = aux;

	sc->sc_dev = self;

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.ub_dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_ncomp = 0;
	if (usbsc->usbsc_ohci_dev != NULL) {
		sc->sc_comps[sc->sc_ncomp++] = usbsc->usbsc_ohci_dev;
	}

	sc->sc_id_vendor = PCI_VENDOR_BROADCOM;
	strlcpy(sc->sc_vendor, "Broadcom", sizeof(sc->sc_vendor));

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": ECHI USB controller\n");

	int error = ehci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}
	/* Attach usb device. */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
}

/*
 * There's only IRQ shared between both OCHI and EHCI devices.
 */
static int
bcmusb_intr(void *arg)
{
	struct bcmusb_softc * const usbsc = arg;
	int rv0 = 0, rv1 = 0;

	if (usbsc->usbsc_ohci_sc)
		rv0 = ohci_intr(usbsc->usbsc_ohci_sc);

	if (usbsc->usbsc_ehci_sc)
		rv1 = ehci_intr(usbsc->usbsc_ehci_sc);

	return rv0 ? rv0 : rv1;
}

static int bcmusb_ccb_match(device_t, cfdata_t, void *);
static void bcmusb_ccb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmusb_ccb, sizeof(struct bcmusb_softc),
	bcmusb_ccb_match, bcmusb_ccb_attach, NULL, NULL);

int
bcmusb_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name) != 0)
		return 0;

	KASSERT(cf->cf_loc[BCMCCBCF_PORT] == BCMCCBCF_PORT_DEFAULT);

	return 1;
}

#define	OHCI_OFFSET	(OHCI_BASE - EHCI_BASE)

void
bcmusb_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmusb_softc * const usbsc = device_private(self);
	const struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	usbsc->usbsc_bst = ccbaa->ccbaa_ccb_bst;
	usbsc->usbsc_dmat = ccbaa->ccbaa_dmat;

	bus_space_subregion(usbsc->usbsc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, 0x1000, &usbsc->usbsc_ehci_bsh);
	bus_space_subregion(usbsc->usbsc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset + OHCI_OFFSET, 0x1000, &usbsc->usbsc_ohci_bsh);

	/*
	 * Bring the PHYs out of reset.
	 */
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ehci_bsh,
	    USBH_PHY_CTRL_P0, USBH_PHY_CTRL_INIT);
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ehci_bsh,
	    USBH_PHY_CTRL_P1, USBH_PHY_CTRL_INIT);

	/*
	 * Disable interrupts
	 */
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ohci_bsh,
	    OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
	bus_size_t caplength = bus_space_read_1(usbsc->usbsc_bst,
	    usbsc->usbsc_ehci_bsh, EHCI_CAPLENGTH);
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ehci_bsh,
	    caplength + EHCI_USBINTR, 0);

	aprint_naive("\n");
	aprint_normal("\n");

	struct bcmusb_attach_args usbaa_ohci = {
		.usbaa_name = "ohci",
		.usbaa_dmat = usbsc->usbsc_dmat,
		.usbaa_bst = usbsc->usbsc_bst,
		.usbaa_bsh = usbsc->usbsc_ohci_bsh,
		.usbaa_size = 0x100,
	};

	usbsc->usbsc_ohci_dev = config_found(self, &usbaa_ohci, NULL);
	if (usbsc->usbsc_ohci_dev != NULL)
		usbsc->usbsc_ohci_sc = device_private(usbsc->usbsc_ohci_dev);

	struct bcmusb_attach_args usbaa_ehci = {
		.usbaa_name = "ehci",
		.usbaa_dmat = usbsc->usbsc_dmat,
		.usbaa_bst = usbsc->usbsc_bst,
		.usbaa_bsh = usbsc->usbsc_ehci_bsh,
		.usbaa_size = 0x100,
	};

	usbsc->usbsc_ehci_dev = config_found(self, &usbaa_ehci, NULL);
	if (usbsc->usbsc_ehci_dev != NULL)
		usbsc->usbsc_ehci_sc = device_private(usbsc->usbsc_ehci_dev);

	usbsc->usbsc_ih = intr_establish(loc->loc_intrs[0], IPL_USB, IST_LEVEL,
	    bcmusb_intr, usbsc);
	if (usbsc->usbsc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intrs[0]);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intrs[0]);
}
