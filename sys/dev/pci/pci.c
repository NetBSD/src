/*	$NetBSD: pci.c,v 1.45 2000/03/22 00:47:26 cgd Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997, 1998
 *     Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
 */

/*
 * PCI bus autoconfiguration.
 */

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#ifdef PCI_CONFIG_DUMP
int pci_config_dump = 1;
#else
int pci_config_dump = 0;
#endif

int pcimatch __P((struct device *, struct cfdata *, void *));
void pciattach __P((struct device *, struct device *, void *));

struct pci_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot, sc_memt;
	bus_dma_tag_t sc_dmat;
	pci_chipset_tag_t sc_pc;
	int sc_bus, sc_maxndevs;
	u_int sc_intrswiz;
	pcitag_t sc_intrtag;
	int sc_flags;
};

struct cfattach pci_ca = {
	sizeof(struct pci_softc), pcimatch, pciattach
};

void	pci_probe_bus __P((struct device *));
int	pciprint __P((void *, const char *));
int	pcisubmatch __P((struct device *, struct cfdata *, void *));

/*
 * Important note about PCI-ISA bridges:
 *
 * Callbacks are used to configure these devices so that ISA/EISA bridges
 * can attach their child busses after PCI configuration is done.
 *
 * This works because:
 *	(1) there can be at most one ISA/EISA bridge per PCI bus, and
 *	(2) any ISA/EISA bridges must be attached to primary PCI
 *	    busses (i.e. bus zero).
 *
 * That boils down to: there can only be one of these outstanding
 * at a time, it is cleared when configuring PCI bus 0 before any
 * subdevices have been found, and it is run after all subdevices
 * of PCI bus 0 have been found.
 *
 * This is needed because there are some (legacy) PCI devices which
 * can show up as ISA/EISA devices as well (the prime example of which
 * are VGA controllers).  If you attach ISA from a PCI-ISA/EISA bridge,
 * and the bridge is seen before the video board is, the board can show
 * up as an ISA device, and that can (bogusly) complicate the PCI device's
 * attach code, or make the PCI device not be properly attached at all.
 *
 * We use the generic config_defer() facility to achieve this.
 */

int
pcimatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;

	if (strcmp(pba->pba_busname, cf->cf_driver->cd_name))
		return (0);

	/* Check the locators */
	if (cf->pcibuscf_bus != PCIBUS_UNK_BUS &&
	    cf->pcibuscf_bus != pba->pba_bus)
		return (0);

	/* sanity */
	if (pba->pba_bus < 0 || pba->pba_bus > 255)
		return (0);

	/*
	 * XXX check other (hardware?) indicators
	 */

	return 1;
}

void
pci_probe_bus(self)
	struct device *self;
{
	struct pci_softc *sc = (struct pci_softc *)self;
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	const struct pci_quirkdata *qd;
	int bus, device, maxndevs, function, nfunctions;

	iot = sc->sc_iot;
	memt = sc->sc_memt;
	pc = sc->sc_pc;
	bus = sc->sc_bus;
	maxndevs = sc->sc_maxndevs;

	for (device = 0; device < maxndevs; device++) {
		pcitag_t tag;
		pcireg_t id, class, intr, bhlcr, csr;
		struct pci_attach_args pa;
		int pin;

		tag = pci_make_tag(pc, bus, device, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0)
			continue;

		qd = pci_lookup_quirkdata(PCI_VENDOR(id), PCI_PRODUCT(id));

		bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_MULTIFN(bhlcr) ||
		    (qd != NULL &&
		      (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0))
			nfunctions = 8;
		else
			nfunctions = 1;

		for (function = 0; function < nfunctions; function++) {
			tag = pci_make_tag(pc, bus, device, function);
			id = pci_conf_read(pc, tag, PCI_ID_REG);
			csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
			class = pci_conf_read(pc, tag, PCI_CLASS_REG);
			intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(id) == 0)
				continue;

			pa.pa_iot = iot;
			pa.pa_memt = memt;
			pa.pa_dmat = sc->sc_dmat;
			pa.pa_pc = pc;
			pa.pa_device = device;
			pa.pa_function = function;
			pa.pa_tag = tag;
			pa.pa_id = id;
			pa.pa_class = class;

			/*
			 * Set up memory, I/O enable, and PCI command flags
			 * as appropriate.
			 */
			pa.pa_flags = sc->sc_flags;
			if ((csr & PCI_COMMAND_IO_ENABLE) == 0)
				pa.pa_flags &= ~PCI_FLAGS_IO_ENABLED;
			if ((csr & PCI_COMMAND_MEM_ENABLE) == 0)
				pa.pa_flags &= ~PCI_FLAGS_MEM_ENABLED;

			if (bus == 0) {
				pa.pa_intrswiz = 0;
				pa.pa_intrtag = tag;
			} else {
				pa.pa_intrswiz = sc->sc_intrswiz + device;
				pa.pa_intrtag = sc->sc_intrtag;
			}
			pin = PCI_INTERRUPT_PIN(intr);
			if (pin == PCI_INTERRUPT_PIN_NONE) {
				/* no interrupt */
				pa.pa_intrpin = 0;
			} else {
				/*
				 * swizzle it based on the number of
				 * busses we're behind and our device
				 * number.
				 */
				pa.pa_intrpin =			/* XXX */
				    ((pin + pa.pa_intrswiz - 1) % 4) + 1;
			}
			pa.pa_intrline = PCI_INTERRUPT_LINE(intr);

			config_found_sm(self, &pa, pciprint, pcisubmatch);
		}
	}
}

void
pciattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;
	struct pci_softc *sc = (struct pci_softc *)self;
	int io_enabled, mem_enabled, mrl_enabled, mrm_enabled, mwi_enabled;
	const char *sep = "";

	pci_attach_hook(parent, self, pba);
	printf("\n");

	io_enabled = (pba->pba_flags & PCI_FLAGS_IO_ENABLED);
	mem_enabled = (pba->pba_flags & PCI_FLAGS_MEM_ENABLED);
	mrl_enabled = (pba->pba_flags & PCI_FLAGS_MRL_OKAY);
	mrm_enabled = (pba->pba_flags & PCI_FLAGS_MRM_OKAY);
	mwi_enabled = (pba->pba_flags & PCI_FLAGS_MWI_OKAY);

	if (io_enabled == 0 && mem_enabled == 0) {
		printf("%s: no spaces enabled!\n", self->dv_xname);
		return;
	}

#define	PRINT(s)	do { printf("%s%s", sep, s); sep = ", "; } while (0)

	printf("%s: ", self->dv_xname);

	if (io_enabled)
		PRINT("i/o space");
	if (mem_enabled)
		PRINT("memory space");
	printf(" enabled");

	if (mrl_enabled || mrm_enabled || mwi_enabled) {
		if (mrl_enabled)
			PRINT("rd/line");
		if (mrm_enabled)
			PRINT("rd/mult");
		if (mwi_enabled)
			PRINT("wr/inv");
		printf(" ok");
	}

	printf("\n");

#undef PRINT

	sc->sc_iot = pba->pba_iot;
	sc->sc_memt = pba->pba_memt;
	sc->sc_dmat = pba->pba_dmat;
	sc->sc_pc = pba->pba_pc;
	sc->sc_bus = pba->pba_bus;
	sc->sc_maxndevs = pci_bus_maxdevs(pba->pba_pc, pba->pba_bus);
	sc->sc_intrswiz = pba->pba_intrswiz;
	sc->sc_intrtag = pba->pba_intrtag;
	sc->sc_flags = pba->pba_flags;

	pci_probe_bus(self);
}

int
pciprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pci_attach_args *pa = aux;
	char devinfo[256];
	const struct pci_quirkdata *qd;

	if (pnp) {
		pci_devinfo(pa->pa_id, pa->pa_class, 1, devinfo);
		printf("%s at %s", devinfo, pnp);
	}
	printf(" dev %d function %d", pa->pa_device, pa->pa_function);
	if (pci_config_dump) {
		printf(": ");
		pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
		if (!pnp)
			pci_devinfo(pa->pa_id, pa->pa_class, 1, devinfo);
		printf("%s at %s", devinfo, pnp ? pnp : "?");
		printf(" dev %d function %d (", pa->pa_device, pa->pa_function);
#ifdef __i386__
		printf("tag %#lx, intrtag %#lx, intrswiz %#lx, intrpin %#lx",
		    *(long *)&pa->pa_tag, *(long *)&pa->pa_intrtag,
		    (long)pa->pa_intrswiz, (long)pa->pa_intrpin);
#else
		printf("tag %#lx, intrtag %#lx, intrswiz %#lx, intrpin %#lx",
		    (long)pa->pa_tag, (long)pa->pa_intrtag, (long)pa->pa_intrswiz,
		    (long)pa->pa_intrpin);
#endif
		printf(", i/o %s, mem %s,",
		    pa->pa_flags & PCI_FLAGS_IO_ENABLED ? "on" : "off",
		    pa->pa_flags & PCI_FLAGS_MEM_ENABLED ? "on" : "off");
		qd = pci_lookup_quirkdata(PCI_VENDOR(pa->pa_id),
		    PCI_PRODUCT(pa->pa_id));
		if (qd == NULL) {
			printf(" no quirks");
		} else {
			bitmask_snprintf(qd->quirks,
			    "\20\1multifn", devinfo, sizeof (devinfo));
			printf(" quirks %s", devinfo);
		}
		printf(")");
	}
	return (UNCONF);
}

int
pcisubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (cf->pcicf_dev != PCI_UNK_DEV &&
	    cf->pcicf_dev != pa->pa_device)
		return 0;
	if (cf->pcicf_function != PCI_UNK_FUNCTION &&
	    cf->pcicf_function != pa->pa_function)
		return 0;
	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
pci_get_capability(pc, tag, capid, offset, value)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int capid;
	int *offset;
	pcireg_t *value;
{
	pcireg_t reg;
	unsigned int ofs;

	reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if (!(reg & PCI_STATUS_CAPLIST_SUPPORT))
		return (0);

	ofs = PCI_CAPLIST_PTR(pci_conf_read(pc, tag, PCI_CAPLISTPTR_REG));
	while (ofs != 0) {
#ifdef DIAGNOSTIC
		if ((ofs & 3) || (ofs < 0x40))
			panic("pci_get_capability");
#endif
		reg = pci_conf_read(pc, tag, ofs);
		if (PCI_CAPLIST_CAP(reg) == capid) {
			if (offset)
				*offset = ofs;
			if (value)
				*value = reg;
			return (1);
		}
		ofs = PCI_CAPLIST_NEXT(reg);
	}

	return (0);
}
