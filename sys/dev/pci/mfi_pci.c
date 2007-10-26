/* $NetBSD: mfi_pci.c,v 1.2.26.1 2007/10/26 15:46:33 joerg Exp $ */
/* $OpenBSD: mfi_pci.c,v 1.11 2006/08/06 04:40:08 brad Exp $ */
/*
 * Copyright (c) 2006 Marco Peereboom <marco@peereboom.us>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mfi_pci.c,v 1.2.26.1 2007/10/26 15:46:33 joerg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/bus.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/mfireg.h>
#include <dev/ic/mfivar.h>

#define	MFI_BAR		0x10
#define	MFI_PCI_MEMSIZE	0x2000 /* 8k */

int	mfi_pci_find_device(void *);
int	mfi_pci_match(struct device *, struct cfdata *, void *);
void	mfi_pci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mfi_pci, sizeof(struct mfi_softc),
    mfi_pci_match, mfi_pci_attach, NULL, NULL);

struct	mfi_pci_device {
	pcireg_t	mpd_vendor;
	pcireg_t	mpd_product;
	pcireg_t	mpd_subvendor;
	pcireg_t	mpd_subproduct;
	const char	*mpd_model;
	uint32_t	mpd_flags;
};

static const
struct  mfi_pci_device mfi_pci_devices[] = {
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_MEGARAID_SAS,
	  0,			0,		"",			0 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_MEGARAID_VERDE_ZCR,
	  0,			0,		"",			0 },
	{ PCI_VENDOR_DELL,	PCI_PRODUCT_DELL_PERC_5,
	  PCI_VENDOR_DELL,	0x1f01,		"Dell PERC 5/e",	0 },
	{ PCI_VENDOR_DELL,	PCI_PRODUCT_DELL_PERC_5,
	  PCI_VENDOR_DELL,	0x1f02,		"Dell PERC 5/i",	0 },
	{ 0, 0, 
	  0, 0, NULL, 0}
};

int
mfi_pci_find_device(void *aux) {
	struct pci_attach_args	*pa = aux;
	int			i;

	for (i = 0; mfi_pci_devices[i].mpd_vendor; i++) {
		if (mfi_pci_devices[i].mpd_vendor == PCI_VENDOR(pa->pa_id) &&
		    mfi_pci_devices[i].mpd_product == PCI_PRODUCT(pa->pa_id)) {
		    	DNPRINTF(MFI_D_MISC, "mfi_pci_find_device: %i\n", i);
			return (i);
		}
	}

	return (-1);
}

int
mfi_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	int			i;

	if ((i = mfi_pci_find_device(aux)) != -1) {
		DNPRINTF(MFI_D_MISC,
		    "mfi_pci_match: vendor: %04x  product: %04x\n",
		    mfi_pci_devices[i].mpd_vendor,
		    mfi_pci_devices[i].mpd_product);

		return (1);
	}

	return (0);
}

void
mfi_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct mfi_softc	*sc = (struct mfi_softc *)self;
	struct pci_attach_args	*pa = aux;
	const char		*intrstr;
	pci_intr_handle_t	ih;
	bus_size_t		size;
	pcireg_t		csr;
	uint32_t		subsysid, i;

	subsysid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	for (i = 0; mfi_pci_devices[i].mpd_vendor; i++)
		if (mfi_pci_devices[i].mpd_subvendor == PCI_VENDOR(subsysid) &&
		    mfi_pci_devices[i].mpd_subproduct == PCI_PRODUCT(subsysid)){
				aprint_normal(", %s",
				    mfi_pci_devices[i].mpd_model);
				break;
		}

	csr = pci_mapreg_type(pa->pa_pc, pa->pa_tag, MFI_BAR);
	csr |= PCI_MAPREG_MEM_TYPE_32BIT;
	if (pci_mapreg_map(pa, MFI_BAR, csr, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &size)) {
		aprint_error(": can't map controller pci space\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	if (pci_intr_map(pa, &ih)) {
		aprint_error(": can't map interrupt\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, mfi_intr, sc);
	if (!sc->sc_ih) {
		aprint_error(": can't establish interrupt");
		if (intrstr)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
		return;
	}

	aprint_normal(": %s\n", intrstr);

	if (mfi_attach(sc)) {
		aprint_error("%s: can't attach", DEVNAME(sc));
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		sc->sc_ih = NULL;
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
	}
}
