/*	$NetBSD: ofpci.c,v 1.3 2002/05/13 13:59:21 matt Exp $	*/

/*
 * Copyright (c) 2002 Eduardo Horvath.  All rights reserved.
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
 * OF PCI bus autoconfiguration.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofpci.c,v 1.3 2002/05/13 13:59:21 matt Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <sparc64/dev/ofpcivar.h>

extern int pci_config_dump;

#ifdef	DEBUG
int ofpci_debug = 0;
#define DPRINTF(l, s)	do { if (ofpci_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

int ofpcimatch __P((struct device *, struct cfdata *, void *));
void ofpciattach __P((struct device *, struct device *, void *));
void ofpci_attach_device __P((struct device *, int, pci_chipset_tag_t, int));

/* We use these for simplicity. */
extern int	pciprint __P((void *, const char *));
extern int	pcisubmatch __P((struct device *, struct cfdata *, void *));

struct cfattach ofpci_ca = {
	sizeof(struct ofpci_softc), ofpcimatch, ofpciattach
};

static char *getname(int );

static char *
getname(int node) 
{
	static char name[30];

	name[0] = name[29] = 0;
	OF_getprop(node, "name", name, sizeof(name));
	return (name);
}

int
ofpcimatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;

	DPRINTF(1, ("ofpcimatch: pba %s cd %s\n",
		pba->pba_busname, cf->cf_driver->cd_name));

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
	DPRINTF(1, ("ofpcimatch: success\n"));

	return 1;
}

void
ofpci_attach_device(struct device *self, int node,
	pci_chipset_tag_t pc, int bus)
{
	struct ofpci_softc *sc = (struct ofpci_softc *)self;
	struct pci_attach_args pa;
	pcitag_t tag;
	int id, class, intr, bhlcr, csr;
	int device, function;
	struct ofw_pci_register reg;

	if (OF_getprop(node, "reg", (void *)&reg, sizeof(reg)) < sizeof(reg))
		panic("ofpciattach: %s regs too small\n", getname(node));

	if (bus != OFW_PCI_PHYS_HI_BUS(reg.phys_hi))
		panic("ofpciattach: %s wrong bus", getname(node));
	device = OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi);
	function = OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi);

	tag = ofpci_make_tag(pc, node, bus, device, function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);

	if (OF_getprop(node, "class-code", &class, sizeof(class))
		!= sizeof(class))
		return;

	class = pci_conf_read(pc, tag, PCI_CLASS_REG);

	/* We may not have any interrupts */
	intr = 0;
	OF_getprop(node, "interrupts", &intr, sizeof(intr));


	/* Invalid vendor ID value? */
	if (PCI_VENDOR(id) == PCI_VENDOR_INVALID || PCI_VENDOR(id) == 0)
		panic("ofpciattach: %s invalid vendor", getname(node));

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);

	pa.pa_iot = sc->sc_pcis.sc_iot;
	pa.pa_memt = sc->sc_pcis.sc_memt;
	pa.pa_dmat = sc->sc_pcis.sc_dmat;
	pa.pa_pc = pc;
	pa.pa_bus = bus;
	pa.pa_device = device;
	pa.pa_function = function;
	pa.pa_tag = tag;
	pa.pa_id = id;
	pa.pa_class = class;

	/*
	 * Set up memory, I/O enable, and PCI command flags
	 * as appropriate.
	 */
	pa.pa_flags = sc->sc_pcis.sc_flags;
	if ((csr & PCI_COMMAND_IO_ENABLE) == 0)
		pa.pa_flags &= ~PCI_FLAGS_IO_ENABLED;
	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0)
		pa.pa_flags &= ~PCI_FLAGS_MEM_ENABLED;

	/*
	 * If the cache line size is not configured, then
	 * clear the MRL/MRM/MWI command-ok flags.
	 */
	if (PCI_CACHELINE(bhlcr) == 0)
		pa.pa_flags &= ~(PCI_FLAGS_MRL_OKAY|
			PCI_FLAGS_MRM_OKAY|PCI_FLAGS_MWI_OKAY);

	/*
	 * XXXX
	 * We'll just store the interrupts property in the pa_intrswiz
	 * field since it happens to be the right size.
	 */
	pa.pa_intrswiz = intr;

	config_found_sm(self, &pa, pciprint, pcisubmatch);
}

void
ofpciattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;
	struct ofpcibus_attach_args *opba = aux;
	struct ofpci_softc *sc = (struct ofpci_softc *)self;
	int io_enabled, mem_enabled, mrl_enabled, mrm_enabled, mwi_enabled;
	int bus;
	pci_chipset_tag_t pc;
	int node;
	const char *sep = "";

DPRINTF(1, ("ofpciattach: entry\n"));
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

	sc->sc_pcis.sc_iot = pba->pba_iot;
	sc->sc_pcis.sc_memt = pba->pba_memt;
	sc->sc_pcis.sc_dmat = pba->pba_dmat;
	sc->sc_pcis.sc_pc = pba->pba_pc;
	sc->sc_pcis.sc_bus = pba->pba_bus;
	sc->sc_pcis.sc_maxndevs = pci_bus_maxdevs(pba->pba_pc, pba->pba_bus);
	sc->sc_pcis.sc_intrswiz = pba->pba_intrswiz;
	sc->sc_pcis.sc_intrtag = pba->pba_intrtag;
	sc->sc_pcis.sc_flags = pba->pba_flags;
	sc->sc_node = opba->opba_node;

	/* Now walk the OF device tree and attach all child nodes. */
	pc = sc->sc_pcis.sc_pc;
	bus = sc->sc_pcis.sc_bus;
	for (node = OF_child(sc->sc_node); node != 0 && node != -1; 
	     node = OF_peer(node)) {
		DPRINTF(1, ("ofpciattach: attaching %s\n", getname(node)));
		ofpci_attach_device(self, node, pc, bus);
	}
}

