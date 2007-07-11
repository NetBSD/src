/*	$NetBSD: ahcisata_pci.c,v 1.1.6.2 2007/07/11 20:06:58 mjf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ahcisata_pci.c,v 1.1.6.2 2007/07/11 20:06:58 mjf Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/disklabel.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/ic/ahcisatavar.h>

static int  ahci_pci_match(struct device *, struct cfdata *, void *);
static void ahci_pci_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ahcisata_pci, sizeof(struct ahci_softc),
    ahci_pci_match, ahci_pci_attach, NULL, NULL);

static int
ahci_pci_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pci_attach_args *pa = aux;
	bus_space_tag_t regt;
	bus_space_handle_t regh;
	bus_size_t size;
	int ret = 0;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_SATA &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_SATA_AHCI) {
		/* check if the chip is in ahci mode */
		if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
		    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
		    &regt, &regh, NULL, &size) != 0)
			return (0);
		if (bus_space_read_4(regt, regh, AHCI_GHC) & AHCI_GHC_AE)
			ret = 3;
		bus_space_unmap(regt, regh, size);
		return (3);
	}

	return (ret);
}

static void
ahci_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct ahci_softc *sc = (struct ahci_softc *)self;
	bus_size_t size;
	char devinfo[256];
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	void *ih;

	if (pci_mapreg_map(pa, AHCI_PCI_ABAR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_ahcit, &sc->sc_ahcih, NULL, &size) != 0) {
		aprint_error("%s: can't map ahci registers\n", AHCINAME(sc));
		return;
	}
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
	ahci_attach(sc);
}
