/*	$NetBSD: if_gem_pci.c,v 1.1.2.6 2002/06/20 03:45:25 nathanw Exp $ */

/*
 * 
 * Copyright (C) 2001 Eduardo Horvath.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * PCI bindings for Sun GEM ethernet controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_gem_pci.c,v 1.1.2.6 2002/06/20 03:45:25 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/endian.h>

#include <uvm/uvm_extern.h>
 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/gemreg.h>
#include <dev/ic/gemvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/* XXX Should use Properties when that's fleshed out. */
#ifdef macppc
#include <dev/ofw/openfirm.h>
#endif /* macppc */

struct gem_pci_softc {
	struct	gem_softc	gsc_gem;	/* GEM device */
	void			*gsc_ih;
};

int	gem_match_pci __P((struct device *, struct cfdata *, void *));
void	gem_attach_pci __P((struct device *, struct device *, void *));

struct cfattach gem_pci_ca = {
	sizeof(struct gem_pci_softc), gem_match_pci, gem_attach_pci
};

/*
 * Attach routines need to be split out to different bus-specific files.
 */

int
gem_match_pci(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN && 
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_ERINETWORK ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_GEMNETWORK))
		return (1);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE && 
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC2))
		return (1);


	return (0);
}

void
gem_attach_pci(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct gem_pci_softc *gsc = (void *)self;
	struct gem_softc *sc = &gsc->gsc_gem;
	pci_intr_handle_t ih;
	const char *intrstr;
	char devinfo[256];
	uint8_t enaddr[ETHER_ADDR_LEN];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	sc->sc_dmatag = pa->pa_dmat;

	sc->sc_pci = 1;		/* XXX */

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN && 
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_ERINETWORK ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_GEMNETWORK))
		sc->sc_variant = GEM_SUN_GEM;
	else if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE && 
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC2))
		sc->sc_variant = GEM_APPLE_GMAC;

#define PCI_GEM_BASEADDR	(PCI_MAPREG_START + 0x00)

	/* XXX Need to check for a 64-bit mem BAR? */
	if (pci_mapreg_map(pa, PCI_GEM_BASEADDR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_bustag, &sc->sc_h, NULL, NULL) != 0)
	{
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * XXX This should be done with properties, when those are
	 * XXX fleshed out.
	 */
#ifdef __sparc__
	{
		extern void myetheraddr __P((u_char *));
		myetheraddr(enaddr);
	}
#endif /* __sparc__ */
#ifdef macppc
	{
		int node;

		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == 0) {
			printf("%s: unable to locate OpenFirmware node\n",
			    sc->sc_dev.dv_xname);
			return;
		}

		OF_getprop(node, "local-mac-address", enaddr, sizeof(enaddr));
	}
#endif /* macppc */

	if (pci_intr_map(pa, &ih) != 0) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}	
	intrstr = pci_intr_string(pa->pa_pc, ih);
	gsc->gsc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_NET, gem_intr, sc);
	if (gsc->gsc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* Finish off the attach. */
	gem_attach(sc, enaddr);
}
