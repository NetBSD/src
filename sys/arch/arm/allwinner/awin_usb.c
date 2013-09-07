/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
#include "ohci.h"
#include "ehci.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_usb.c,v 1.4 2013/09/07 02:32:53 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#if NOHCI > 0
#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>
#endif

#if NEHCI > 0
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#endif

#include <dev/pci/pcidevs.h>

struct awinusb_softc {
	device_t usbsc_dev;
	bus_dma_tag_t usbsc_dmat;
	bus_space_tag_t usbsc_bst;
	bus_space_handle_t usbsc_ehci_bsh;
	bus_space_handle_t usbsc_ohci_bsh;

	device_t usbsc_ohci_dev;
	device_t usbsc_ehci_dev;
	void *usbsc_ohci_ih;
	void *usbsc_ehci_ih;
};

struct awinusb_attach_args {
	const char *usbaa_name;
	bus_dma_tag_t usbaa_dmat;
	bus_space_tag_t usbaa_bst;
	bus_space_handle_t usbaa_bsh;
	bus_space_handle_t usbaa_ccm_bsh;
	bus_size_t usbaa_size;
	int usbaa_port;
};

#if NOHCI > 0
static const int awinusb_ohci_irqs[2] = { AWIN_IRQ_USB3, AWIN_IRQ_USB4 };
static const uint32_t awinusb_ohci_ahb_gating[2] = {
	AWIN_AHB_GATING0_USB_OHCI0, AWIN_AHB_GATING0_USB_OHCI1,
};
static const uint32_t awinusb_ohci_usb_clk_set[2] = {
	AWIN_USB_CLK_OHCI0_ENABLE|AWIN_USB_CLK_USBPHY_ENABLE|AWIN_USB_CLK_PHY1_ENABLE,
	AWIN_USB_CLK_OHCI1_ENABLE|AWIN_USB_CLK_USBPHY_ENABLE|AWIN_USB_CLK_PHY2_ENABLE,
};


#ifdef OHCI_DEBUG
#define OHCI_DPRINTF(x)	if (ohcidebug) printf x
extern int ohcidebug;
#else
#define OHCI_DPRINTF(x)
#endif

static int ohci_awinusb_match(device_t, cfdata_t, void *);
static void ohci_awinusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_awinusb, sizeof(struct ohci_softc),
	ohci_awinusb_match, ohci_awinusb_attach, NULL, NULL);

static int
ohci_awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ohci_awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(parent);
	struct ohci_softc * const sc = device_private(self);
	struct awinusb_attach_args * const usbaa = aux;

	sc->sc_dev = self;

	awin_reg_set_clear(usbaa->usbaa_bst, usbaa->usbaa_ccm_bsh,
	    AWIN_AHB_GATING0_REG, awinusb_ohci_ahb_gating[usbaa->usbaa_port],
	    0);
	awin_reg_set_clear(usbaa->usbaa_bst, usbaa->usbaa_ccm_bsh,
	    AWIN_USB_CLK_REG, awinusb_ohci_usb_clk_set[usbaa->usbaa_port], 0);

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.hci_private = sc;

	//sc->sc_id_vendor = PCI_VENDOR_ALLWINNER;
	strlcpy(sc->sc_vendor, "Allwinner", sizeof(sc->sc_vendor));

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller\n");

	int error = ohci_init(sc);
	if (error != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
	} else {
		/* Attach usb device. */
		sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	}

	const int irq = awinusb_ohci_irqs[usbaa->usbaa_port];
	usbsc->usbsc_ohci_ih = intr_establish(irq, IPL_USB,
	    IST_LEVEL, ohci_intr, sc);
	if (usbsc->usbsc_ohci_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", irq);
}
#endif /* NOHCI > 0 */

#if NEHCI > 0
#ifdef EHCI_DEBUG
#define EHCI_DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define EHCI_DPRINTF(x)
#endif

static int ehci_awinusb_match(device_t, cfdata_t, void *);
static void ehci_awinusb_attach(device_t, device_t, void *);

static const int awinusb_ehci_irqs[2] = { AWIN_IRQ_USB1, AWIN_IRQ_USB2 };
static const uint32_t awinusb_ehci_ahb_gating[2] = {
	AWIN_AHB_GATING0_USB_EHCI0, AWIN_AHB_GATING0_USB_EHCI1,
};

CFATTACH_DECL_NEW(ehci_awinusb, sizeof(struct ehci_softc),
	ehci_awinusb_match, ehci_awinusb_attach, NULL, NULL);

static int
ehci_awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ehci_awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(parent);
	struct ehci_softc * const sc = device_private(self);
	struct awinusb_attach_args * const usbaa = aux;

	sc->sc_dev = self;

	awin_reg_set_clear(usbaa->usbaa_bst, usbaa->usbaa_ccm_bsh,
	    AWIN_AHB_GATING0_REG, awinusb_ehci_ahb_gating[usbaa->usbaa_port],
	    0);

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.hci_private = sc;
	sc->sc_bus.usbrev = USBREV_2_0;
	sc->sc_ncomp = 0;
	if (usbsc->usbsc_ohci_dev != NULL) {
		sc->sc_comps[sc->sc_ncomp++] = usbsc->usbsc_ohci_dev;
	}

	//sc->sc_id_vendor = PCI_VENDOR_ALLWINNER;
	strlcpy(sc->sc_vendor, "Allwinner", sizeof(sc->sc_vendor));

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": ECHI USB controller\n");

	int error = ehci_init(sc);
	if (error != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
	} else {
		/* Attach usb device. */
		sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	}

	const int irq = awinusb_ehci_irqs[usbaa->usbaa_port];
	usbsc->usbsc_ehci_ih = intr_establish(irq, IPL_USB,
	    IST_LEVEL, ehci_intr, sc);
	if (usbsc->usbsc_ehci_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", irq);
}
#endif /* NEHCI > 0 */

static int awinusb_match(device_t, cfdata_t, void *);
static void awinusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(awin_usb, sizeof(struct awinusb_softc),
	awinusb_match, awinusb_attach, NULL, NULL);

static int awinusb_ports;

int
awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	KASSERT(loc->loc_port != AWINIOCF_PORT_DEFAULT);
	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT 
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);
	KASSERT((awinusb_ports & __BIT(loc->loc_port)) == 0);

	return 1;
}

void
awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(self);
	const struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	awinusb_ports |= __BIT(loc->loc_port);
	usbsc->usbsc_bst = aio->aio_core_bst;
	usbsc->usbsc_dmat = aio->aio_dmat;

	bus_space_subregion(usbsc->usbsc_bst, aio->aio_core_bsh,
	    loc->loc_offset + AWIN_EHCI_OFFSET, AWIN_EHCI_SIZE,
	    &usbsc->usbsc_ehci_bsh);
	bus_space_subregion(usbsc->usbsc_bst, aio->aio_core_bsh,
	    loc->loc_offset + AWIN_OHCI_OFFSET, AWIN_OHCI_SIZE,
	    &usbsc->usbsc_ohci_bsh);

	/*
	 * Disable interrupts
	 */
#if NOHCI > 0
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ohci_bsh,
	    OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
#endif
#if NEHCI > 0
	bus_size_t caplength = bus_space_read_1(usbsc->usbsc_bst,
	    usbsc->usbsc_ehci_bsh, EHCI_CAPLENGTH);
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ehci_bsh,
	    caplength + EHCI_USBINTR, 0);
#endif

	aprint_naive("\n");
	aprint_normal("\n");

#if NOHCI > 0
	struct awinusb_attach_args usbaa_ohci = {
		.usbaa_name = "ohci",
		.usbaa_dmat = usbsc->usbsc_dmat,
		.usbaa_bst = usbsc->usbsc_bst,
		.usbaa_bsh = usbsc->usbsc_ohci_bsh,
		.usbaa_ccm_bsh = aio->aio_ccm_bsh,
		.usbaa_size = AWIN_OHCI_SIZE,
		.usbaa_port = loc->loc_port,
	};

	usbsc->usbsc_ohci_dev = config_found(self, &usbaa_ohci, NULL);
#endif

#if NEHCI > 0
	struct awinusb_attach_args usbaa_ehci = {
		.usbaa_name = "ehci",
		.usbaa_dmat = usbsc->usbsc_dmat,
		.usbaa_bst = usbsc->usbsc_bst,
		.usbaa_bsh = usbsc->usbsc_ehci_bsh,
		.usbaa_ccm_bsh = aio->aio_ccm_bsh,
		.usbaa_size = AWIN_EHCI_SIZE,
		.usbaa_port = loc->loc_port,
	};

	config_found(self, &usbaa_ehci, NULL);
#endif
}
