/*	$NetBSD: ehci_pci.c,v 1.28 2007/07/08 18:22:28 jmcneill Exp $	*/

/*
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
__KERNEL_RCSID(0, "$NetBSD: ehci_pci.c,v 1.28 2007/07/08 18:22:28 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

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

static void ehci_get_ownership(ehci_softc_t *sc, pci_chipset_tag_t pc,
			       pcitag_t tag);
static void ehci_pci_powerhook(int, void *);

struct ehci_pci_softc {
	ehci_softc_t		sc;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void 			*sc_ih;		/* interrupt vectoring */

	void			*sc_powerhook;
	struct pci_conf_state	sc_pciconf;
};

#define EHCI_MAX_BIOS_WAIT		1000 /* ms */

static int
ehci_pci_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_EHCI)
		return (1);

	return (0);
}

static void
ehci_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ehci_pci_softc *sc = (struct ehci_pci_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	const char *vendor;
	const char *devname = sc->sc.sc_bus.bdev.dv_xname;
	char devinfo[256];
	usbd_status r;
	int ncomp;
	struct usb_pci *up;

	aprint_naive(": USB controller\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_CBMEM, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc.sc_size)) {
		aprint_error("%s: can't map memory space\n", devname);
		return;
	}

	sc->sc_pc = pc;
	sc->sc_tag = tag;
	sc->sc.sc_bus.dmatag = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

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

	switch(pci_conf_read(pc, tag, PCI_USBREV) & PCI_USBREV_MASK) {
	case PCI_USBREV_PRE_1_0:
	case PCI_USBREV_1_0:
	case PCI_USBREV_1_1:
		sc->sc.sc_bus.usbrev = USBREV_UNKNOWN;
		aprint_verbose("%s: pre-2.0 USB rev\n", devname);
		return;
	case PCI_USBREV_2_0:
		sc->sc.sc_bus.usbrev = USBREV_2_0;
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

	/* Enable workaround for dropped interrupts as required */
	if (sc->sc.sc_id_vendor == PCI_VENDOR_VIATECH)
		sc->sc.sc_flags |= EHCIF_DROPPED_INTR_WORKAROUND;

	/*
	 * Find companion controllers.  According to the spec they always
	 * have lower function numbers so they should be enumerated already.
	 */
	ncomp = 0;
	TAILQ_FOREACH(up, &ehci_pci_alldevs, next) {
		if (up->bus == pa->pa_bus && up->device == pa->pa_device) {
			DPRINTF(("ehci_pci_attach: companion %s\n",
				 USBDEVNAME(up->usb->bdev)));
			sc->sc.sc_comps[ncomp++] = up->usb;
			if (ncomp >= EHCI_COMPANION_MAX)
				break;
		}
	}
	sc->sc.sc_ncomp = ncomp;

	ehci_get_ownership(&sc->sc, pc, tag);

	r = ehci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error("%s: init failed, error=%d\n", devname, r);
		return;
	}

	sc->sc_powerhook = powerhook_establish(
	    USBDEVNAME(sc->sc.sc_bus.bdev) , ehci_pci_powerhook, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: couldn't establish powerhook\n",
		    devname);

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

static int
ehci_pci_detach(device_ptr_t self, int flags)
{
	struct ehci_pci_softc *sc = (struct ehci_pci_softc *)self;
	int rv;

	if (sc->sc_powerhook != NULL)
		powerhook_disestablish(sc->sc_powerhook);

	rv = ehci_detach(&sc->sc, flags);
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
	return (0);
}

CFATTACH_DECL(ehci_pci, sizeof(struct ehci_pci_softc),
    ehci_pci_match, ehci_pci_attach, ehci_pci_detach, ehci_activate);

#ifdef EHCI_DEBUG
static void
ehci_dump_caps(ehci_softc_t *sc, pci_chipset_tag_t pc, pcitag_t tag)
{
	u_int32_t cparams, legctlsts, addr, cap, id;
	int maxdump = 10;

	cparams = EREAD4(sc, EHCI_HCCPARAMS);
	addr = EHCI_HCC_EECP(cparams);
	while (addr != 0) {
		cap = pci_conf_read(pc, tag, addr);
		id = EHCI_CAP_GET_ID(cap);
		switch (id) {
		case EHCI_CAP_ID_LEGACY:
			legctlsts = pci_conf_read(pc, tag,
						  addr + PCI_EHCI_USBLEGCTLSTS);
			printf("ehci_dump_caps: legsup=0x%08x "
			       "legctlsts=0x%08x\n", cap, legctlsts);
			break;
		default:
			printf("ehci_dump_caps: cap=0x%08x\n", cap);
			break;
		}
		if (--maxdump < 0)
			break;
		addr = EHCI_CAP_GET_NEXT(cap);
	}
}
#endif

static void
ehci_get_ownership(ehci_softc_t *sc, pci_chipset_tag_t pc, pcitag_t tag)
{
	const char *devname = sc->sc_bus.bdev.dv_xname;
	u_int32_t cparams, addr, cap, legsup;
	int maxcap = 10;
	int ms;

#ifdef EHCI_DEBUG
	if (ehcidebug)
		ehci_dump_caps(sc, pc, tag);
#endif
	cparams = EREAD4(sc, EHCI_HCCPARAMS);
	addr = EHCI_HCC_EECP(cparams);
	while (addr != 0) {
		cap = pci_conf_read(pc, tag, addr);
		if (EHCI_CAP_GET_ID(cap) == EHCI_CAP_ID_LEGACY)
			break;
		if (--maxcap < 0) {
			aprint_normal("%s: broken extended capabilities "
				      "ignored\n", devname);
			return;
		}
		addr = EHCI_CAP_GET_NEXT(cap);
	}

	/* If the USB legacy capability is not specified, we are done */
	if (addr == 0)
		return;

	legsup = pci_conf_read(pc, tag, addr + PCI_EHCI_USBLEGSUP);
	/* Ask BIOS to give up ownership */
	legsup |= EHCI_LEG_HC_OS_OWNED;
	pci_conf_write(pc, tag, addr + PCI_EHCI_USBLEGSUP, legsup);
	for (ms = 0; ms < EHCI_MAX_BIOS_WAIT; ms++) {
		legsup = pci_conf_read(pc, tag, addr + PCI_EHCI_USBLEGSUP);
		if (!(legsup & EHCI_LEG_HC_BIOS_OWNED))
			break;
		delay(1000);
	}
	if (ms == EHCI_MAX_BIOS_WAIT) {
		aprint_normal("%s: BIOS refuses to give up ownership, "
			      "using force\n", devname);
		pci_conf_write(pc, tag, addr + PCI_EHCI_USBLEGSUP, 0);
		pci_conf_write(pc, tag, addr + PCI_EHCI_USBLEGCTLSTS, 0);
	} else {
		aprint_verbose("%s: BIOS has given up ownership\n", devname);
	}
}

static void
ehci_pci_powerhook(int why, void *opaque)
{
	struct ehci_pci_softc *sc;
	pci_chipset_tag_t pc;
	pcitag_t tag;

	sc = (struct ehci_pci_softc *)opaque;
	pc = sc->sc_pc;
	tag = sc->sc_tag;

	switch (why) {
	case PWR_STANDBY:
	case PWR_SUSPEND:
		pci_conf_capture(pc, tag, &sc->sc_pciconf);
		break;
	case PWR_RESUME:
		pci_conf_restore(pc, tag, &sc->sc_pciconf);
		ehci_get_ownership(&sc->sc, pc, tag);
		break;
	}

	return;
}
