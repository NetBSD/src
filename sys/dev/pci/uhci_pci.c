/*	$NetBSD: uhci_pci.c,v 1.1 1998/07/12 19:51:58 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/uhcireg.h>
#include <dev/usb/uhcivar.h>

#include <machine/bus.h>

int	uhci_pci_match __P((struct device *, struct cfdata *, void *));
void	uhci_pci_attach __P((struct device *, struct device *, void *));

struct cfattach uhci_pci_ca = {
	sizeof(uhci_softc_t), uhci_pci_match, uhci_pci_attach
};

int
uhci_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;
	if (PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_82371AB_USB &&
	    PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_INTEL_82371SB_USB)
		return 0;

	return 1;
}

void
uhci_pci_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	uhci_softc_t *sc = (uhci_softc_t *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	char *typestr;
	char devinfo[256];
	usbd_status r;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		switch(PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82371SB_USB:
			sc->sc_model = UHCI_PIIX3;
			break;
		case PCI_PRODUCT_INTEL_82371AB_USB:
			sc->sc_model = UHCI_PIIX4;
			break;
#ifdef DIAGNOSTIC
		default:
			printf("uhci_attach: unknown product 0x%x\n",
			       PCI_PRODUCT(pa->pa_id));
			return;
#endif
		}
	}

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
			   &sc->iot, &sc->ioh, NULL, NULL)) {
		printf("%s: can't map i/o space\n", sc->sc_bus.bdev.dv_xname);
		return;
	}

	sc->sc_dmatag = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", 
		       sc->sc_bus.bdev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_USB, uhci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_bus.bdev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_bus.bdev.dv_xname, intrstr);

	switch(pci_conf_read(pc, pa->pa_tag, PCI_USBREV) & PCI_USBREV_MASK) {
	case PCI_USBREV_PRE_1_0:
		typestr = "pre 1.0";
		break;
	case PCI_USBREV_1_0:
		typestr = "1.0";
		break;
	default:
		typestr = "unknown";
		break;
	}
	printf("%s: USB version %s\n", sc->sc_bus.bdev.dv_xname, typestr);

	r = uhci_init(sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", 
		       sc->sc_bus.bdev.dv_xname, r);
		return;
	}

	/* Attach usb device. */
	config_found((void *)sc, &sc->sc_bus, usbctlprint);
}
