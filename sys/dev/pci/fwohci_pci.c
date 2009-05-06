/*	$NetBSD: fwohci_pci.c,v 1.33 2009/05/06 09:25:15 cegger Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fwohci_pci.c,v 1.33 2009/05/06 09:25:15 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/select.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/ieee1394/fw_port.h>
#include <dev/ieee1394/firewire.h>
#include <dev/ieee1394/firewirereg.h>
#include <dev/ieee1394/fwdma.h>
#include <dev/ieee1394/fwohcireg.h>
#include <dev/ieee1394/fwohcivar.h>

struct fwohci_pci_softc {
	struct fwohci_softc psc_sc;

	pci_chipset_tag_t psc_pc;
	pcitag_t psc_tag;

	void *psc_ih;
};

static int fwohci_pci_match(device_t, cfdata_t, void *);
static void fwohci_pci_attach(device_t, device_t, void *);

static bool fwohci_pci_suspend(device_t PMF_FN_PROTO);
static bool fwohci_pci_resume(device_t PMF_FN_PROTO);

CFATTACH_DECL_NEW(fwohci_pci, sizeof(struct fwohci_pci_softc),
    fwohci_pci_match, fwohci_pci_attach, NULL, NULL);

static int
fwohci_pci_match(device_t parent, cfdata_t match,
    void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_FIREWIRE &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_OHCI)
		return 1;

	return 0;
}

static void
fwohci_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;
	struct fwohci_pci_softc *psc = device_private(self);
	char devinfo[256];
	char const *intrstr;
	pci_intr_handle_t ih;
	u_int32_t csr;

	aprint_naive(": IEEE 1394 Controller\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));

	psc->psc_sc.fc.dev = self;
	psc->psc_sc.fc.dmat = pa->pa_dmat;
	psc->psc_pc = pa->pa_pc;
	psc->psc_tag = pa->pa_tag;

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_OHCI_MAP_REGISTER, PCI_MAPREG_TYPE_MEM, 0,
	    &psc->psc_sc.bst, &psc->psc_sc.bsh,
	    NULL, &psc->psc_sc.bssize)) {
		aprint_error_dev(self, "can't map OHCI register space\n");
		goto fail;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	OHCI_CSR_WRITE(&psc->psc_sc, FWOHCI_INTMASKCLR, OHCI_INT_EN);

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		goto fail;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	psc->psc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, fwohci_filt,
	    &psc->psc_sc);
	if (psc->psc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto fail;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	if (!pmf_device_register(self, fwohci_pci_suspend, fwohci_pci_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (fwohci_init(&(psc->psc_sc), psc->psc_sc.fc.dev) != 0) {
		pci_intr_disestablish(pa->pa_pc, psc->psc_ih);
		bus_space_unmap(psc->psc_sc.bst, psc->psc_sc.bsh,
		    psc->psc_sc.bssize);
	}

	return;

fail:
	/* In the event that we fail to attach, register a null pnp handler */
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
}

static bool
fwohci_pci_suspend(device_t dv PMF_FN_ARGS)
{
	struct fwohci_pci_softc *psc = device_private(dv);
	int s;

	s = splbio();
	fwohci_stop(&psc->psc_sc, psc->psc_sc.fc.dev);
	splx(s);

	return true;
}

static bool
fwohci_pci_resume(device_t dv PMF_FN_ARGS)
{
	struct fwohci_pci_softc *psc = device_private(dv);
	int s;

	s = splbio();
	fwohci_resume(&psc->psc_sc, psc->psc_sc.fc.dev);
	splx(s);

	return true;
}
