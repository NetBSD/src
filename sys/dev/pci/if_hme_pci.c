/*	$NetBSD: if_hme_pci.c,v 1.5.2.2 2001/10/11 00:02:10 fvdl Exp $	*/

/*
 * Copyright (c) 2000 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PCI front-end device driver for the HME ethernet device.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hmevar.h>

struct hme_pci_softc {
	struct	hme_softc	hsc_hme;	/* HME device */
	bus_space_tag_t		hsc_memt;
	bus_space_handle_t	hsc_memh;
	void			*hsc_ih;
};

int	hmematch_pci __P((struct device *, struct cfdata *, void *));
void	hmeattach_pci __P((struct device *, struct device *, void *));

struct cfattach hme_pci_ca = {
	sizeof(struct hme_pci_softc), hmematch_pci, hmeattach_pci
};

int
hmematch_pci(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN && 
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_HMENETWORK)
		return (1);

	return (0);
}

void
hmeattach_pci(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct hme_pci_softc *hsc = (void *)self;
	struct hme_softc *sc = &hsc->hsc_hme;
	pci_intr_handle_t ih;
	pcireg_t csr;
	const char *intrstr;
	int type;

	/* XXX the following declarations should be elsewhere */
	extern void myetheraddr __P((u_char *));

	printf(": Sun Happy Meal Ethernet, rev. %d\n",
	    PCI_REVISION(pa->pa_class));

	/*
	 * enable io/memory-space accesses.  this is kinda of gross; but
	 # the hme comes up with neither IO space enabled, or memory space.
	 */
	if (pa->pa_memt)
		pa->pa_flags |= PCI_FLAGS_MEM_ENABLED;
	if (pa->pa_iot)
		pa->pa_flags |= PCI_FLAGS_IO_ENABLED;
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	if (pa->pa_memt) {
		type = PCI_MAPREG_TYPE_MEM;
		csr |= PCI_COMMAND_MEM_ENABLE;
		sc->sc_bustag = pa->pa_memt;
	} else {
		type = PCI_MAPREG_TYPE_IO;
		csr |= PCI_COMMAND_IO_ENABLE;
		sc->sc_bustag = pa->pa_iot;
	}
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MEM_ENABLE);

	sc->sc_dmatag = pa->pa_dmat;

	sc->sc_pci = 1; /* XXXXX should all be done in bus_dma. */
	/*
	 * Map five register banks:
	 *
	 *	bank 0: HME SEB registers:	+0x0000
	 *	bank 1: HME ETX registers:	+0x2000
	 *	bank 2: HME ERX registers:	+0x4000
	 *	bank 3: HME MAC registers:	+0x6000
	 *	bank 4: HME MIF registers:	+0x7000
	 *
	 */

#define PCI_HME_BASEADDR	0x10
	if (pci_mapreg_map(pa, PCI_HME_BASEADDR, type, 0,
	    &hsc->hsc_memt, &hsc->hsc_memh, NULL, NULL) != 0)
	{
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_seb = hsc->hsc_memh;
	if (bus_space_subregion(hsc->hsc_memt, hsc->hsc_memh, 0x2000,
	    0x1000, &sc->sc_etx)) {
		printf("%s: unable to subregion ETX registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_subregion(hsc->hsc_memt, hsc->hsc_memh, 0x4000,
	    0x1000, &sc->sc_erx)) {
		printf("%s: unable to subregion ERX registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_subregion(hsc->hsc_memt, hsc->hsc_memh, 0x6000,
	    0x1000, &sc->sc_mac)) {
		printf("%s: unable to subregion MAC registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_subregion(hsc->hsc_memt, hsc->hsc_memh, 0x7000,
	    0x1000, &sc->sc_mif)) {
		printf("%s: unable to subregion MIF registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	myetheraddr(sc->sc_enaddr);

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &ih) != 0) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}	
	intrstr = pci_intr_string(pa->pa_pc, ih);
	hsc->hsc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_NET, hme_intr, sc);
	if (hsc->hsc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

#if 0
	/*
	 * Get transfer burst size from PROM and pass it on
	 * to the back-end driver.
	 */
	sbusburst = ((struct sbus_softc *)parent)->sc_burst;
	if (sbusburst == 0)
		sbusburst = SBUS_BURST_32 - 1; /* 1->16 */

	burst = PROM_getpropint(node, "burst-sizes", -1);
	if (burst == -1)
		/* take SBus burst sizes */
		burst = sbusburst;

	/* Clamp at parent's burst sizes */
	burst &= sbusburst;

	/* Translate into plain numerical format */
	sc->sc_burst =  (burst & SBUS_BURST_32) ? 32 :
			(burst & SBUS_BURST_16) ? 16 : 0;
#endif

	sc->sc_burst = 16;	/* XXX */

	/* Finish off the attach. */
	hme_config(sc);
}
