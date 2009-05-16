/*	$NetBSD: fwlynx_pci.c,v 1.13.4.2 2009/05/16 10:41:33 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fwlynx_pci.c,v 1.13.4.2 2009/05/16 10:41:33 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/fwlynxreg.h>
#include <dev/ieee1394/fwlynxvar.h>

struct fwlynx_pci_softc {
	struct fwlynx_softc psc_sc;
	pci_chipset_tag_t psc_pc;
	void *psc_ih;
};

static int fwlynx_pci_match(device_t, cfdata_t, void *);
static void fwlynx_pci_attach(device_t, device_t, void *);

CFATTACH_DECL(fwlynx_pci, sizeof(struct fwlynx_pci_softc),
    fwlynx_pci_match, fwlynx_pci_attach, NULL, NULL);

static int
fwlynx_pci_match(device_t parent, cfdata_t match, void *aux)
{
        struct pci_attach_args *pa = (struct pci_attach_args *) aux;

        if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_TI &&
            PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_TI_TSB12LV21)
                return 1;

        return 0;
}

static void
fwlynx_pci_attach(device_t parent, device_t self, void *aux)
{
        struct pci_attach_args *pa = (struct pci_attach_args *) aux;
	struct fwlynx_pci_softc *psc = device_private(self);
        char devinfo[256];
	char const *intrstr;
	pci_intr_handle_t ih;
	u_int32_t csr;

	aprint_naive(": IEEE 1394 Controller\n");

        pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
        aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
            PCI_REVISION(pa->pa_class));

	psc->psc_sc.sc_dmat = pa->pa_dmat;
	psc->psc_pc = pa->pa_pc;
	psc->psc_tag = pa->pa_tag;

        /* Map I/O registers */
        if (pci_mapreg_map(pa, PCI_LYNX_MAP_REGISTER, PCI_MAPREG_TYPE_MEM, 0,
                           &psc->psc_sc.sc_memt, &psc->psc_sc.sc_memh,
			   NULL, &psc->psc_sc.sc_memsize)) {
                aprint_error_dev(self, "can't map register space\n");
                return;
        }

        /* Disable interrupts, so we don't get any spurious ones. */
	pci_conf_write(psc->psc_pc, psc->psc_tag, 0);

        /* Enable the device. */
        csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
        pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
                       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
        	aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	psc->psc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
	    fwlynx_intr, &psc->psc_sc);
	if (psc->psc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	if (fwlynx_init(&psc->psc_sc, pci_intr_evcnt(pa->pa_pc, ih)) != 0) {
		pci_intr_disestablish(pa->pa_pc, psc->psc_ih);
		bus_space_unmap(psc->psc_sc.sc_memt, psc->psc_sc.sc_memh,
		    psc->psc_sc.sc_memsize);
	}
}
