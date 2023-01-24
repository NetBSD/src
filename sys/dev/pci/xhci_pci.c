/*	$NetBSD: xhci_pci.c,v 1.33 2023/01/24 08:45:47 mlelstv Exp $	*/
/*	OpenBSD: xhci_pci.c,v 1.4 2014/07/12 17:38:51 yuo Exp	*/

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
__KERNEL_RCSID(0, "$NetBSD: xhci_pci.c,v 1.33 2023/01/24 08:45:47 mlelstv Exp $");

#ifdef _KERNEL_OPT
#include "opt_xhci_pci.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <sys/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

struct xhci_pci_softc {
	struct xhci_softc	sc_xhci;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;
	void			*sc_ih;
	pci_intr_handle_t	*sc_pihp;
};

static int
xhci_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_XHCI)
		return 1;

	return 0;
}

static int
xhci_pci_port_route(struct xhci_pci_softc *psc)
{
	struct xhci_softc * const sc = &psc->sc_xhci;

	pcireg_t val;

	/*
	 * Check USB3 Port Routing Mask register that indicates the ports
	 * can be changed from OS, and turn on by USB3 Port SS Enable register.
	 */
	val = pci_conf_read(psc->sc_pc, psc->sc_tag, PCI_XHCI_INTEL_USB3PRM);
	aprint_debug_dev(sc->sc_dev,
	    "USB3PRM / USB3.0 configurable ports: 0x%08x\n", val);

	pci_conf_write(psc->sc_pc, psc->sc_tag, PCI_XHCI_INTEL_USB3_PSSEN, val);
	val = pci_conf_read(psc->sc_pc, psc->sc_tag,PCI_XHCI_INTEL_USB3_PSSEN);
	aprint_debug_dev(sc->sc_dev,
	    "USB3_PSSEN / Enabled USB3.0 ports under xHCI: 0x%08x\n", val);

	/*
	 * Check USB2 Port Routing Mask register that indicates the USB2.0
	 * ports to be controlled by xHCI HC, and switch them to xHCI HC.
	 */
	val = pci_conf_read(psc->sc_pc, psc->sc_tag, PCI_XHCI_INTEL_USB2PRM);
	aprint_debug_dev(sc->sc_dev,
	    "XUSB2PRM / USB2.0 ports can switch from EHCI to xHCI:"
	    "0x%08x\n", val);
	pci_conf_write(psc->sc_pc, psc->sc_tag, PCI_XHCI_INTEL_XUSB2PR, val);
	val = pci_conf_read(psc->sc_pc, psc->sc_tag, PCI_XHCI_INTEL_XUSB2PR);
	aprint_debug_dev(sc->sc_dev,
	    "XUSB2PR / USB2.0 ports under xHCI: 0x%08x\n", val);

	return 0;
}

static void
xhci_pci_attach(device_t parent, device_t self, void *aux)
{
	struct xhci_pci_softc * const psc = device_private(self);
	struct xhci_softc * const sc = &psc->sc_xhci;
	struct pci_attach_args *const pa = (struct pci_attach_args *)aux;
	const pci_chipset_tag_t pc = pa->pa_pc;
	const pcitag_t tag = pa->pa_tag;
	char const *intrstr;
	pcireg_t csr, memtype, usbrev;
	uint32_t hccparams;
	int counts[PCI_INTR_TYPE_SIZE];
	char intrbuf[PCI_INTRSTR_LEN];
	bus_addr_t memaddr;
	int flags, msixoff;
	int err;

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, "USB Controller");

	/* Check for quirks */
	sc->sc_quirks = 0;

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0) {
		/*
		 * Enable address decoding for memory range in case BIOS or
		 * UEFI didn't set it.
		 */
		csr = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_MEM_ENABLE;
		pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		    csr);
	}

	/* map MMIO registers */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_CBMEM);
	if (PCI_MAPREG_TYPE(memtype) != PCI_MAPREG_TYPE_MEM) {
		sc->sc_ios = 0;
		aprint_error_dev(self, "BAR not 64 or 32-bit MMIO\n");
		return;
	}

	sc->sc_iot = pa->pa_memt;
	if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, PCI_CBMEM, memtype,
	    &memaddr, &sc->sc_ios, &flags) != 0) {
		sc->sc_ios = 0;
		aprint_error_dev(self, "can't get map info\n");
		return;
	}

	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSIX, &msixoff,
	    NULL)) {
		pcireg_t msixtbl;
		uint32_t table_offset;
		int bir;

		msixtbl = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    msixoff + PCI_MSIX_TBLOFFSET);
		table_offset = msixtbl & PCI_MSIX_TBLOFFSET_MASK;
		bir = msixtbl & PCI_MSIX_TBLBIR_MASK;
		/* Shrink map area for MSI-X table */
		if (bir == PCI_MAPREG_NUM(PCI_CBMEM))
			sc->sc_ios = table_offset;
	}
	if (bus_space_map(sc->sc_iot, memaddr, sc->sc_ios, flags,
	    &sc->sc_ioh)) {
		sc->sc_ios = 0;
		aprint_error_dev(self, "can't map mem space\n");
		return;
	}

	psc->sc_pc = pc;
	psc->sc_tag = tag;

	hccparams = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XHCI_HCCPARAMS);

	if (XHCI_HCC_AC64(hccparams) != 0) {
		aprint_verbose_dev(self, "64-bit DMA");
		if (pci_dma64_available(pa)) {
			sc->sc_bus.ub_dmatag = pa->pa_dmat64;
			aprint_verbose("\n");
		} else {
			aprint_verbose(" - limited\n");
			sc->sc_bus.ub_dmatag = pa->pa_dmat;
		}
	} else {
		aprint_verbose_dev(self, "32-bit DMA\n");
		sc->sc_bus.ub_dmatag = pa->pa_dmat;
	}

	/* Enable the device. */
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	for (int i = 0; i < PCI_INTR_TYPE_SIZE; i++) {
		counts[i] = 1;
	}
#ifdef XHCI_DISABLE_MSI
	counts[PCI_INTR_TYPE_MSI] = 0;
#endif
#ifdef XHCI_DISABLE_MSIX
	counts[PCI_INTR_TYPE_MSIX] = 0;
#endif

	/* Allocate and establish the interrupt. */
	if (pci_intr_alloc(pa, &psc->sc_pihp, counts, PCI_INTR_TYPE_MSIX)) {
		aprint_error_dev(self, "can't allocate handler\n");
		goto fail;
	}
	intrstr = pci_intr_string(pc, psc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));
	pci_intr_setattr(pc, &psc->sc_pihp[0], PCI_INTR_MPSAFE, true);
	psc->sc_ih = pci_intr_establish_xname(pc, psc->sc_pihp[0], IPL_USB,
	    xhci_intr, sc, device_xname(sc->sc_dev));
	if (psc->sc_ih == NULL) {
		pci_intr_release(pc, psc->sc_pihp, 1);
		psc->sc_pihp = NULL;
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto fail;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	usbrev = pci_conf_read(pc, tag, PCI_USBREV) & PCI_USBREV_MASK;
	switch (usbrev) {
	case PCI_USBREV_3_0:
		sc->sc_bus.ub_revision = USBREV_3_0;
		break;
	case PCI_USBREV_3_1:
		sc->sc_bus.ub_revision = USBREV_3_1;
		break;
	default:
		if (usbrev < PCI_USBREV_3_0) {
			aprint_error_dev(self, "Unknown revision (%02x). Set to 3.0.\n",
			    usbrev);
			sc->sc_bus.ub_revision = USBREV_3_0;
		} else {
			/* Default to the latest revision */
			aprint_normal_dev(self,
			    "Unknown revision (%02x). Set to 3.1.\n", usbrev);
			sc->sc_bus.ub_revision = USBREV_3_1;
		}
		break;
	}

	/* Intel chipset requires SuperSpeed enable and USB2 port routing */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		sc->sc_quirks |= XHCI_QUIRK_INTEL;
		break;
	default:
		break;
	}

	err = xhci_init(sc);
	if (err) {
		aprint_error_dev(self, "init failed, error=%d\n", err);
		goto fail;
	}

	if ((sc->sc_quirks & XHCI_QUIRK_INTEL) != 0)
		xhci_pci_port_route(psc);

	if (!pmf_device_register1(self, xhci_suspend, xhci_resume,
	                          xhci_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");

	/* Attach usb buses. */
	if (sc->sc_usb3nports != 0)
		sc->sc_child =
		    config_found(self, &sc->sc_bus, usbctlprint, CFARGS_NONE);

	if (sc->sc_usb2nports != 0)
		sc->sc_child2 =
		    config_found(self, &sc->sc_bus2, usbctlprint, CFARGS_NONE);

	return;

fail:
	if (psc->sc_ih != NULL) {
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);
		psc->sc_ih = NULL;
	}
	if (psc->sc_pihp != NULL) {
		pci_intr_release(psc->sc_pc, psc->sc_pihp, 1);
		psc->sc_pihp = NULL;
	}
	if (sc->sc_ios) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
		sc->sc_ios = 0;
	}
	return;
}

static int
xhci_pci_detach(device_t self, int flags)
{
	struct xhci_pci_softc * const psc = device_private(self);
	struct xhci_softc * const sc = &psc->sc_xhci;
	int rv;

	if (sc->sc_ios != 0) {
		rv = xhci_detach(sc, flags);
		if (rv)
			return rv;

		pmf_device_deregister(self);

		xhci_shutdown(self, flags);

#if 0
		/* Disable interrupts, so we don't get any spurious ones. */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
#endif
	}

	if (psc->sc_ih != NULL) {
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);
		psc->sc_ih = NULL;
	}
	if (psc->sc_pihp != NULL) {
		pci_intr_release(psc->sc_pc, psc->sc_pihp, 1);
		psc->sc_pihp = NULL;
	}
	if (sc->sc_ios) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
		sc->sc_ios = 0;
	}

	return 0;
}

CFATTACH_DECL3_NEW(xhci_pci, sizeof(struct xhci_pci_softc),
    xhci_pci_match, xhci_pci_attach, xhci_pci_detach, xhci_activate, NULL,
    xhci_childdet, DVF_DETACH_SHUTDOWN);
