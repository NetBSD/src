/*	$NetBSD: simba.c,v 1.1 1999/06/04 13:42:15 mrg Exp $	*/

/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * from: NetBSD: ppb.c,v 1.18 1998/06/08 06:55:57 thorpej Exp
 */

/*
 * this is a copy of the pci/ppb driver slightly modified to
 * work with the SUNW,simba PCI bus.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <sparc64/dev/simbareg.h>

static	int	simba_match __P((struct device *, struct cfdata *, void *));
static	void	simba_attach __P((struct device *, struct device *, void *));
static	int	simba_print __P((void *, const char *pnp));
extern	struct sparc_pci_chipset sparc_simba_pci_chipset;

struct cfattach simba_ca = {
	sizeof(struct device), simba_match, simba_attach
};

static int
simba_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_SIMBA)
		return (1);

	return (0);
}

/*
 * XXX
 *
 * this code depends on the parent PCI bus's parent being the sabre!
 */
static void
simba_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct psycho_softc *sc = (struct psycho_softc *)parent->dv_parent;	/* XXX */
	struct psycho_pbm *pp;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct pcibus_attach_args pba;
	pcireg_t busdata;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	busdata = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);

	/*
	 * this uses the generic PCI bus value's, rather than the `bus-range'
	 * property; that's cuz this code was stolen from dev/pci/ppb.c.  we
	 * might need to change to that yet.  getting the prom-node might be
	 * fun, but we *do* have pa->pa_pc->cookie->...
	 *
	 * perhaps it would be all better to probe the simba's and get all
	 * the info ourselves.. we'll see.
	 */
	switch (PPB_BUSINFO_SECONDARY(busdata)) {
	default:
		printf("%s: not configured by system firmware (bus %d)\n",
		    self->dv_xname, PPB_BUSINFO_SECONDARY(busdata));
		return;
	case 1:					/* PCI B */
		pp = sc->sc_simba_b;
		break;
	case 2:					/* PCI A */
		pp = sc->sc_simba_a;
		break;
	}

	/*
	 * attach the PCI bus than hangs off of it.  we use the
	 * per-psycho values.
	 */
	pba.pba_busname = "pci";
	pba.pba_iot = pp->pp_iot;
	pba.pba_memt = pp->pp_memt;
	pba.pba_dmat = pp->pp_dmat;
	pba.pba_pc = pp->pp_pc;
	pba.pba_flags = pp->pp_flags;
	pba.pba_bus = PPB_BUSINFO_SECONDARY(busdata);
	pba.pba_intrswiz = pa->pa_intrswiz;
	pba.pba_intrtag = pa->pa_intrtag;

	config_found(self, &pba, simba_print);
}

static int
simba_print(aux, p)
	void *aux;
	const char *p;
{
	struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to sabre's; easy. */
	if (p)
		printf("pci at %s", p);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
