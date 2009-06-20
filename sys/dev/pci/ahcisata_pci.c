/*	$NetBSD: ahcisata_pci.c,v 1.11.4.2 2009/06/20 07:20:23 yamt Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ahcisata_pci.c,v 1.11.4.2 2009/06/20 07:20:23 yamt Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/disklabel.h>
#include <sys/pmf.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/ic/ahcisatavar.h>

#define AHCI_PCI_QUIRK_FORCE	1	/* force attach */

static const struct pci_quirkdata ahci_pci_quirks[] = {
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA2,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA3,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA4,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_SATA,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_1,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_MARVELL, PCI_PRODUCT_MARVELL_88SE6121,
	    AHCI_PCI_QUIRK_FORCE },
};

struct ahci_pci_softc {
	struct ahci_softc ah_sc;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
};


static int  ahci_pci_match(device_t, cfdata_t, void *);
static void ahci_pci_attach(device_t, device_t, void *);
const struct pci_quirkdata *ahci_pci_lookup_quirkdata(pci_vendor_id_t,
						      pci_product_id_t);
static bool ahci_pci_resume(device_t PMF_FN_PROTO);


CFATTACH_DECL_NEW(ahcisata_pci, sizeof(struct ahci_pci_softc),
    ahci_pci_match, ahci_pci_attach, NULL, NULL);

static int
ahci_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	bus_space_tag_t regt;
	bus_space_handle_t regh;
	bus_size_t size;
	int ret = 0;
	const struct pci_quirkdata *quirks;

	quirks = ahci_pci_lookup_quirkdata(PCI_VENDOR(pa->pa_id),
					   PCI_PRODUCT(pa->pa_id));

	/* if wrong class and not forced by quirks, don't match */
	if ((PCI_CLASS(pa->pa_class) != PCI_CLASS_MASS_STORAGE ||
	    ((PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_SATA ||
	     PCI_INTERFACE(pa->pa_class) != PCI_INTERFACE_SATA_AHCI) &&
	     PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_RAID)) &&
	    (quirks == NULL || (quirks->quirks & AHCI_PCI_QUIRK_FORCE) == 0))
		return 0;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &regt, &regh, NULL, &size) != 0)
		return 0;

	if ((PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_SATA &&
	     PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_SATA_AHCI) ||
	    (quirks && quirks->quirks & AHCI_PCI_QUIRK_FORCE) ||
	    (bus_space_read_4(regt, regh, AHCI_GHC) & AHCI_GHC_AE))
		ret = 3;

	bus_space_unmap(regt, regh, size);
	return ret;
}

static void
ahci_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ahci_pci_softc *psc = device_private(self);
	struct ahci_softc *sc = &psc->ah_sc;
	bus_size_t size;
	char devinfo[256];
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	void *ih;

	sc->sc_atac.atac_dev = self;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_ahcit, &sc->sc_ahcih, NULL, &size) != 0) {
		aprint_error_dev(self, "can't map ahci registers\n");
		return;
	}
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_naive(": AHCI disk controller\n");
	aprint_normal(": %s\n", devinfo);
	
	if (pci_intr_map(pa, &intrhandle) != 0) {
		aprint_error("%s: couldn't map interrupt\n", AHCINAME(sc));
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle);
	ih = pci_intr_establish(pa->pa_pc, intrhandle, IPL_BIO, ahci_intr, sc);
	if (ih == NULL) {
		aprint_error("%s: couldn't establish interrupt", AHCINAME(sc));
		return;
	}
	aprint_normal("%s: interrupting at %s\n", AHCINAME(sc),
	    intrstr ? intrstr : "unknown interrupt");
	sc->sc_dmat = pa->pa_dmat;

	if (PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_RAID) {
		AHCIDEBUG_PRINT(("%s: RAID mode\n", AHCINAME(sc)), DEBUG_PROBE);
		sc->sc_atac_capflags = ATAC_CAP_RAID;
	} else {
		AHCIDEBUG_PRINT(("%s: SATA mode\n", AHCINAME(sc)), DEBUG_PROBE);
	}

	ahci_attach(sc);

	if (!pmf_device_register(self, NULL, ahci_pci_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
ahci_pci_resume(device_t dv PMF_FN_ARGS)
{
	struct ahci_pci_softc *psc = device_private(dv);
	struct ahci_softc *sc = &psc->ah_sc;
	int s;

	s = splbio();
	ahci_reset(sc);
	ahci_setup_ports(sc);
	ahci_reprobe_drives(sc);
	ahci_enable_intrs(sc);
	splx(s);

	return true;
}

const struct pci_quirkdata *
ahci_pci_lookup_quirkdata(pci_vendor_id_t vendor, pci_product_id_t product)
{
	int i;

	for (i = 0; i < (sizeof ahci_pci_quirks / sizeof ahci_pci_quirks[0]);
	     i++)
		if (vendor == ahci_pci_quirks[i].vendor &&
		    product == ahci_pci_quirks[i].product)
			return (&ahci_pci_quirks[i]);
	return (NULL);
}
