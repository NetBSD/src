/*	$NetBSD: ahcisata_pci.c,v 1.23 2010/11/13 13:52:05 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ahcisata_pci.c,v 1.23 2010/11/13 13:52:05 uebayasi Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/disklabel.h>
#include <sys/pmf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/ic/ahcisatavar.h>

struct ahci_pci_quirk { 
	pci_vendor_id_t  vendor;	/* Vendor ID */
	pci_product_id_t product;	/* Product ID */
	int              quirks;	/* quirks; see below */
};

#define AHCI_PCI_QUIRK_FORCE	__BIT(0)	/* force attach */
#define AHCI_PCI_QUIRK_BAD64	__BIT(1)	/* broken 64-bit DMA */

static const struct ahci_pci_quirk ahci_pci_quirks[] = {
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
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_1,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_2,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_3,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_4,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_5,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_6,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_7,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_8,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_9,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_10,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_11,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_12,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_MARVELL, PCI_PRODUCT_MARVELL_88SE6121,
	    AHCI_PCI_QUIRK_FORCE },
	/* ATI SB600 AHCI 64-bit DMA only works on some boards/BIOSes */
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB600_SATA_1,
	    AHCI_PCI_QUIRK_BAD64 },
};

struct ahci_pci_softc {
	struct ahci_softc ah_sc;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	void * sc_ih;
};

static bool ahci_pci_has_quirk(pci_vendor_id_t, pci_product_id_t, int);
static int  ahci_pci_match(device_t, cfdata_t, void *);
static void ahci_pci_attach(device_t, device_t, void *);
static int  ahci_pci_detach(device_t, int);
static bool ahci_pci_resume(device_t, const pmf_qual_t *);


CFATTACH_DECL_NEW(ahcisata_pci, sizeof(struct ahci_pci_softc),
    ahci_pci_match, ahci_pci_attach, ahci_pci_detach, NULL);

static bool
ahci_pci_has_quirk(pci_vendor_id_t vendor, pci_product_id_t product, int quirk)
{
	int i;

	for (i = 0; i < __arraycount(ahci_pci_quirks); i++)
		if (vendor == ahci_pci_quirks[i].vendor &&
		    product == ahci_pci_quirks[i].product)
			return (ahci_pci_quirks[i].quirks & quirk) != 0;
	return false;
}

static int
ahci_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	bus_space_tag_t regt;
	bus_space_handle_t regh;
	bus_size_t size;
	int ret = 0;
	bool force;

	force = ahci_pci_has_quirk(PCI_VENDOR(pa->pa_id),
				   PCI_PRODUCT(pa->pa_id),
				   AHCI_PCI_QUIRK_FORCE);

	/* if wrong class and not forced by quirks, don't match */
	if ((PCI_CLASS(pa->pa_class) != PCI_CLASS_MASS_STORAGE ||
	    ((PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_SATA ||
	     PCI_INTERFACE(pa->pa_class) != PCI_INTERFACE_SATA_AHCI) &&
	     PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_RAID)) &&
	    (force == false))
		return 0;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &regt, &regh, NULL, &size) != 0)
		return 0;

	if ((PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_SATA &&
	     PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_SATA_AHCI) ||
	    (bus_space_read_4(regt, regh, AHCI_GHC) & AHCI_GHC_AE) ||
	    (force == true))
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
	char devinfo[256];
	const char *intrstr;
	bool ahci_cap_64bit;
	bool ahci_bad_64bit;
	pci_intr_handle_t intrhandle;

	sc->sc_atac.atac_dev = self;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_ahcit, &sc->sc_ahcih, NULL, &sc->sc_ahcis) != 0) {
		aprint_error_dev(self, "can't map ahci registers\n");
		return;
	}
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_naive(": AHCI disk controller\n");
	aprint_normal(": %s\n", devinfo);
	
	if (pci_intr_map(pa, &intrhandle) != 0) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle);
	psc->sc_ih = pci_intr_establish(pa->pa_pc, intrhandle, IPL_BIO, ahci_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n",
	    intrstr ? intrstr : "unknown interrupt");

	sc->sc_dmat = pa->pa_dmat;

	ahci_cap_64bit = (AHCI_READ(sc, AHCI_CAP) & AHCI_CAP_64BIT) != 0;
	ahci_bad_64bit = ahci_pci_has_quirk(PCI_VENDOR(pa->pa_id),
					    PCI_PRODUCT(pa->pa_id),
					    AHCI_PCI_QUIRK_BAD64);

	if (pci_dma64_available(pa) && ahci_cap_64bit) {
		if (!ahci_bad_64bit)
			sc->sc_dmat = pa->pa_dmat64;
		aprint_verbose_dev(self, "64-bit DMA%s\n",
		    (sc->sc_dmat == pa->pa_dmat) ? " unavailable" : "");
	}

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

static int
ahci_pci_detach(device_t dv, int flags)
{
	struct ahci_pci_softc *psc;
	struct ahci_softc *sc;
	int rv;

	psc = device_private(dv);
	sc = &psc->ah_sc;

	if ((rv = ahci_detach(sc, flags)))
		return rv;

	if (psc->sc_ih != NULL)
		pci_intr_disestablish(psc->sc_pc, psc->sc_ih);

	bus_space_unmap(sc->sc_ahcit, sc->sc_ahcih, sc->sc_ahcis);

	return 0;
}

static bool
ahci_pci_resume(device_t dv, const pmf_qual_t *qual)
{
	struct ahci_pci_softc *psc = device_private(dv);
	struct ahci_softc *sc = &psc->ah_sc;
	int s;

	s = splbio();
	ahci_resume(sc);
	splx(s);

	return true;
}
