/*	$NetBSD: ahcisata_pci.c,v 1.65 2022/05/31 08:43:15 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ahcisata_pci.c,v 1.65 2022/05/31 08:43:15 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_ahcisata_pci.h"
#endif

#include <sys/types.h>
#include <sys/kmem.h>
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
	int              quirks;	/* quirks; same as sc_ahci_quirks */
};

static const struct ahci_pci_quirk ahci_pci_quirks[] = {
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA2,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA3,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_SATA4,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_AHCI_1,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_AHCI_2,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_AHCI_3,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP65_AHCI_4,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_SATA,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_SATA2,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_SATA3,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_SATA4,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_1,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_2,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_3,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_4,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_5,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_6,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_7,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP67_AHCI_8,
	     AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_1,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_2,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_3,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_4,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_5,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_6,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_7,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_8,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_9,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_10,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_11,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP73_AHCI_12,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_1,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_2,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_3,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_4,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_5,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_6,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_7,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_8,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_9,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_10,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_11,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP77_AHCI_12,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_1,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_2,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_3,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_4,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_5,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_6,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_7,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_8,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_9,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_10,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_11,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_NVIDIA, PCI_PRODUCT_NVIDIA_MCP79_AHCI_12,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_ALI, PCI_PRODUCT_ALI_M5288,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_MARVELL, PCI_PRODUCT_MARVELL_88SE6121,
	    AHCI_PCI_QUIRK_FORCE | AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_MARVELL, PCI_PRODUCT_MARVELL_88SE6145,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_MARVELL2, PCI_PRODUCT_MARVELL2_88SE91XX,
	    AHCI_PCI_QUIRK_FORCE },
	/* ATI SB600 AHCI 64-bit DMA only works on some boards/BIOSes */
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB600_SATA_1,
	    AHCI_PCI_QUIRK_BAD64 | AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB700_SATA_AHCI,
	    AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB700_SATA_RAID,
	    AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB700_SATA_RAID5,
	    AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB700_SATA_AHCI2,
	    AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_ATI, PCI_PRODUCT_ATI_SB700_SATA_STORAGE,
	    AHCI_QUIRK_BADPMP | AHCI_QUIRK_BADNCQ },
	{ PCI_VENDOR_VIATECH, PCI_PRODUCT_VIATECH_VT8237R_SATA,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_VIATECH, PCI_PRODUCT_VIATECH_VT8251_SATA,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_ASMEDIA, PCI_PRODUCT_ASMEDIA_ASM1061_01,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_ASMEDIA, PCI_PRODUCT_ASMEDIA_ASM1061_02,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_ASMEDIA, PCI_PRODUCT_ASMEDIA_ASM1061_11,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_ASMEDIA, PCI_PRODUCT_ASMEDIA_ASM1061_12,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_AMD, PCI_PRODUCT_AMD_HUDSON_SATA,
	    AHCI_PCI_QUIRK_FORCE },
	{ PCI_VENDOR_AMD, PCI_PRODUCT_AMD_HUDSON_SATA_AHCI,
	    AHCI_QUIRK_BADPMP },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801JI_SATA_AHCI,
	    AHCI_QUIRK_BADPMP },

    /* extra delay */
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_C600_AHCI,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_7SER_MO_SATA_AHCI,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_BSW_AHCI,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_8SER_DT_SATA_AHCI,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_9SERIES_SATA_AHCI,
	    AHCI_QUIRK_EXTRA_DELAY },
#if 0
	/*
	 * XXX Non-reproducible failures reported. May need extra-delay quirk.
	 */
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_BAYTRAIL_SATA_AHCI_0,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_BAYTRAIL_SATA_AHCI_1,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801I_SATA_4,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801I_SATA_5,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801I_SATA_6,
	    AHCI_QUIRK_EXTRA_DELAY },
	{ PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82801I_SATA_7,
	    AHCI_QUIRK_EXTRA_DELAY },
#endif
};

struct ahci_pci_softc {
	struct ahci_softc ah_sc;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	pci_intr_handle_t *sc_pihp;
	int sc_nintr;
	void **sc_ih;
};

static int  ahci_pci_has_quirk(pci_vendor_id_t, pci_product_id_t);
static int  ahci_pci_match(device_t, cfdata_t, void *);
static void ahci_pci_attach(device_t, device_t, void *);
static int  ahci_pci_detach(device_t, int);
static void ahci_pci_childdetached(device_t, device_t);
static bool ahci_pci_resume(device_t, const pmf_qual_t *);


CFATTACH_DECL3_NEW(ahcisata_pci, sizeof(struct ahci_pci_softc),
    ahci_pci_match, ahci_pci_attach, ahci_pci_detach, NULL,
    NULL, ahci_pci_childdetached, DVF_DETACH_SHUTDOWN);

#define	AHCI_PCI_ABAR_CAVIUM	0x10

static int
ahci_pci_has_quirk(pci_vendor_id_t vendor, pci_product_id_t product)
{
	int i;

	for (i = 0; i < __arraycount(ahci_pci_quirks); i++)
		if (vendor == ahci_pci_quirks[i].vendor &&
		    product == ahci_pci_quirks[i].product)
			return ahci_pci_quirks[i].quirks;
	return 0;
}

static int
ahci_pci_abar(struct pci_attach_args *pa)
{
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CAVIUM) {
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CAVIUM_THUNDERX_AHCI ||
		    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CAVIUM_THUNDERX_RAID) {
			return AHCI_PCI_ABAR_CAVIUM;
		}
	}

	return AHCI_PCI_ABAR;
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

	force = ((ahci_pci_has_quirk( PCI_VENDOR(pa->pa_id),
	    PCI_PRODUCT(pa->pa_id)) & AHCI_PCI_QUIRK_FORCE) != 0);

	/* if wrong class and not forced by quirks, don't match */
	if ((PCI_CLASS(pa->pa_class) != PCI_CLASS_MASS_STORAGE ||
	    ((PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_SATA ||
	     PCI_INTERFACE(pa->pa_class) != PCI_INTERFACE_SATA_AHCI) &&
	     PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_RAID)) &&
	    (force == false))
		return 0;

	int bar = ahci_pci_abar(pa);
	pcireg_t memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, bar);
	if (pci_mapreg_map(pa, bar, memtype, 0, &regt, &regh, NULL, &size) != 0)
		return 0;

	if ((PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_SATA &&
	     PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_SATA_AHCI) ||
	    (bus_space_read_4(regt, regh, AHCI_GHC) & AHCI_GHC_AE) ||
	    (force == true))
		ret = 3;

	bus_space_unmap(regt, regh, size);
	return ret;
}

static int
ahci_pci_intr_establish(struct ahci_softc *sc, int port)
{
	struct ahci_pci_softc *psc = (struct ahci_pci_softc *)sc;
	device_t self = sc->sc_atac.atac_dev;
	char intrbuf[PCI_INTRSTR_LEN];
	char intr_xname[INTRDEVNAMEBUF];
	const char *intrstr;
	int vec;
	int (*intr_handler)(void *);
	void *intr_arg;

	KASSERT(psc->sc_pihp != NULL);
	KASSERT(psc->sc_nintr > 0);

	snprintf(intr_xname, sizeof(intr_xname), "%s", device_xname(self));

	if (psc->sc_nintr == 1 || sc->sc_ghc_mrsm) {
		/* Only one interrupt, established on vector 0 */
		intr_handler = ahci_intr;
		intr_arg = sc;
		vec = 0;

		if (psc->sc_ih[vec] != NULL) {
			/* Already established, nothing more to do */
			goto out;
		}

	} else {
		/*
		 * Theoretically AHCI device can have less MSI/MSI-X vectors
		 * than supported ports. Hardware is allowed to revert
		 * to single message MSI, but not required to do so.
		 * So handle the case when it did not revert to single MSI.
		 * In this case last available interrupt vector is used
		 * for port == max vector, and all further ports.
		 * This last vector must use the general interrupt handler,
		 * since it needs to be able to handle several ports.
		 * NOTE: such case was never actually observed yet
		 */
		if (sc->sc_atac.atac_nchannels > psc->sc_nintr
		    && port >= (psc->sc_nintr - 1)) {
			intr_handler = ahci_intr;
			intr_arg = sc;
			vec = psc->sc_nintr - 1;

			if (psc->sc_ih[vec] != NULL) {
				/* Already established, nothing more to do */
				goto out;
			}

			if (port == vec) {
				/* Print error once */
				aprint_error_dev(self,
				    "port %d independent interrupt vector not "
				    "available, sharing with further ports",
				    port);
			}
		} else {
			/* Vector according to port */
			KASSERT(port < psc->sc_nintr);
			KASSERT(psc->sc_ih[port] == NULL);
			intr_handler = ahci_intr_port;
			intr_arg = &sc->sc_channels[port];
			vec = port;

			snprintf(intr_xname, sizeof(intr_xname), "%s port%d",
			    device_xname(self), port);
		}
	}

	intrstr = pci_intr_string(psc->sc_pc, psc->sc_pihp[vec], intrbuf,
	    sizeof(intrbuf));
	psc->sc_ih[vec] = pci_intr_establish_xname(psc->sc_pc,
	    psc->sc_pihp[vec], IPL_BIO, intr_handler, intr_arg, intr_xname);
	if (psc->sc_ih[vec] == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto fail;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

out:
	return 0;

fail:
	return EAGAIN;
}

static void
ahci_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ahci_pci_softc *psc = device_private(self);
	struct ahci_softc *sc = &psc->ah_sc;
	bool ahci_cap_64bit;
	bool ahci_bad_64bit;
	pcireg_t reg;

	sc->sc_atac.atac_dev = self;

	int bar = ahci_pci_abar(pa);
	pcireg_t memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, bar);
	if (pci_mapreg_map(pa, bar, memtype, 0, &sc->sc_ahcit, &sc->sc_ahcih,
	    NULL, &sc->sc_ahcis) != 0) {
		aprint_error_dev(self, "can't map ahci registers\n");
		return;
	}
	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_aprint_devinfo(pa, "AHCI disk controller");

	int counts[PCI_INTR_TYPE_SIZE] = {
		[PCI_INTR_TYPE_INTX] = 1,
		[PCI_INTR_TYPE_MSI] = 1,
		[PCI_INTR_TYPE_MSIX] = -1,
	};

	/* Allocate and establish the interrupt. */
	if (pci_intr_alloc(pa, &psc->sc_pihp, counts, PCI_INTR_TYPE_MSIX)) {
		aprint_error_dev(self, "can't allocate handler\n");
		goto fail;
	}

	psc->sc_nintr = counts[pci_intr_type(pa->pa_pc, psc->sc_pihp[0])];
	psc->sc_ih = kmem_zalloc(sizeof(void *) * psc->sc_nintr, KM_SLEEP);
	sc->sc_intr_establish = ahci_pci_intr_establish;

	sc->sc_dmat = pa->pa_dmat;

	sc->sc_ahci_quirks = ahci_pci_has_quirk(PCI_VENDOR(pa->pa_id),
					    PCI_PRODUCT(pa->pa_id));

	ahci_cap_64bit = (AHCI_READ(sc, AHCI_CAP) & AHCI_CAP_64BIT) != 0;
	ahci_bad_64bit = ((sc->sc_ahci_quirks & AHCI_PCI_QUIRK_BAD64) != 0);

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

	reg = pci_conf_read(psc->sc_pc, psc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	reg |= (PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE);
	pci_conf_write(psc->sc_pc, psc->sc_pcitag, PCI_COMMAND_STATUS_REG, reg);

	ahci_attach(sc);

	if (!pmf_device_register(self, NULL, ahci_pci_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
fail:
	if (psc->sc_pihp != NULL) {
		pci_intr_release(psc->sc_pc, psc->sc_pihp, psc->sc_nintr);
		psc->sc_pihp = NULL;
	}
	if (sc->sc_ahcis) {
		bus_space_unmap(sc->sc_ahcit, sc->sc_ahcih, sc->sc_ahcis);
		sc->sc_ahcis = 0;
	}

	return;

}

static void
ahci_pci_childdetached(device_t dv, device_t child)
{
	struct ahci_pci_softc *psc = device_private(dv);
	struct ahci_softc *sc = &psc->ah_sc;

	ahci_childdetached(sc, child);
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

	pmf_device_deregister(dv);

	if (psc->sc_ih != NULL) {
		for (int intr = 0; intr < psc->sc_nintr; intr++) {
			if (psc->sc_ih[intr] != NULL) {
				pci_intr_disestablish(psc->sc_pc,
				    psc->sc_ih[intr]);
				psc->sc_ih[intr] = NULL;
			}
		}

		kmem_free(psc->sc_ih, sizeof(void *) * psc->sc_nintr);
		psc->sc_ih = NULL;
	}

	if (psc->sc_pihp != NULL) {
		pci_intr_release(psc->sc_pc, psc->sc_pihp, psc->sc_nintr);
		psc->sc_pihp = NULL;
	}

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
