/*	$NetBSD: ebus.c,v 1.1 1999/06/04 13:29:13 mrg Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * UltraSPARC 5 and beyond ebus support.
 */

#undef DEBUG
#define DEBUG

#ifdef DEBUG
#define	EDB_PROM	0x1
#define EDB_CHILD	0x2
int ebus_debug = 0;
#define DPRINTF(l, s)   do { if (ebus_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sparc64/dev/ebusreg.h>
#include <sparc64/dev/ebusvar.h>

int	ebus_match __P((struct device *, struct cfdata *, void *));
void	ebus_attach __P((struct device *, struct device *, void *));

struct cfattach ebus_ca = {
	sizeof(struct device), ebus_match, ebus_attach
};

int	ebus_setup_attach_args __P((struct ebus_softc *, int,
	    struct ebus_attach_args *));
void	ebus_destroy_attach_args __P((struct ebus_attach_args *));
int	ebus_print __P((void *, const char *));
int	ebus_find_node __P((struct ebus_softc *, struct pci_attach_args *));

int
ebus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN && 
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_EBUS)
		return (1);

	return (0);
}

/*
 * attach an ebus and all it's children.  this code is modeled
 * after the sbus code which does similar things.
 */
void
ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ebus_softc *sc = (struct ebus_softc *)self;
	struct pci_attach_args *pa = aux;
	struct ebus_attach_args eba;
	struct ebus_interrupt_map_mask *immp;
	int node, nmapmask, rv;
	char devinfo[256];

	printf("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s, revision 0x%02x\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_bustag = pa->pa_memt;
	sc->sc_childbustag = ebus_alloc_bus_tag(sc, PCI_MEMORY_BUS_SPACE);
	sc->sc_dmatag = ebus_alloc_dma_tag(sc, pa->pa_dmat);

	node = ebus_find_node(sc, pa);
	if (node == 0)
		panic("could not find ebus node");

	sc->sc_node = node;

	/*
	 * fill in our softc with information from the prom
	 */
	sc->sc_intmap = sc->sc_range = NULL;
	rv = getprop(node, "interrupt-map", sizeof(struct ebus_interrupt_map),
	    &sc->sc_nintmap, (void **)&sc->sc_intmap);
	if (rv)
		panic("could not get ebus interrupt-map");

	immp = &sc->sc_intmapmask;
	rv = getprop(node, "interrupt-map-mask",
	    sizeof(struct ebus_interrupt_map_mask), &nmapmask,
	    (void **)&immp);
	if (rv)
		panic("could not get ebus interrupt-map-mask");
	if (nmapmask != 1)
		panic("ebus interrupt-map-mask is broken");

	rv = getprop(node, "ranges", sizeof(struct ebus_ranges),
	    &sc->sc_nrange, (void **)&sc->sc_range);
	if (rv)
		panic("could not get ebus ranges");

	/*
	 * now attach all our children
	 */
	DPRINTF(EDB_CHILD, ("ebus node %08x, searching children...\n", node));
	for (node = firstchild(node); node; node = nextsibling(node)) {
		char *name = getpropstring(node, "name");

		if (ebus_setup_attach_args(sc, node, &eba) != 0) {
			printf("ebus_attach: %s: incomplete\n", name);
			continue;
		}
		DPRINTF(EDB_CHILD, ("- found child `%s', attaching\n", eba.ea_name));
		(void)config_found(self, &eba, ebus_print);
		ebus_destroy_attach_args(&eba);
	}
}

int
ebus_setup_attach_args(sc, node, ea)
	struct ebus_softc	*sc;
	int			node;
	struct ebus_attach_args	*ea;
{
	int	n, rv;

	bzero(ea, sizeof(struct ebus_attach_args));
	rv = getprop(node, "name", 1, &n, (void **)&ea->ea_name);
	if (rv != 0)
		return (rv);
	ea->ea_name[n] = '\0';

	ea->ea_node = node;
	ea->ea_bustag = sc->sc_childbustag;
	ea->ea_dmatag = sc->sc_dmatag;

	rv = getprop(node, "reg", sizeof(struct ebus_regs), &ea->ea_nregs,
	    (void **)&ea->ea_regs);
	if (rv)
		return (rv);

	rv = getprop(node, "address", sizeof(u_int32_t), &ea->ea_naddrs,
	    (void **)&ea->ea_vaddrs);
	if (rv != ENOENT) {
		if (rv)
			return (rv);

		if (ea->ea_nregs != ea->ea_naddrs)
			printf("ebus loses: device %s: %d regs and %d addrs\n",
			    ea->ea_name, ea->ea_nregs, ea->ea_naddrs);
	}

	(void)getprop(node, "interrupts", sizeof(u_int32_t), &ea->ea_nintrs,
	    (void **)&ea->ea_intrs);

	return (0);
}

void
ebus_destroy_attach_args(ea)
	struct ebus_attach_args	*ea;
{

	if (ea->ea_name)
		free((void *)ea->ea_name, M_DEVBUF);
	if (ea->ea_regs)
		free((void *)ea->ea_regs, M_DEVBUF);
	if (ea->ea_intrs)
		free((void *)ea->ea_intrs, M_DEVBUF);
	if (ea->ea_vaddrs)
		free((void *)ea->ea_vaddrs, M_DEVBUF);
}

int
ebus_print(aux, p)
	void *aux;
	const char *p;
{
	struct ebus_attach_args *ea = aux;

	if (p)
		printf("%s at %s", ea->ea_name, p);
	return (UNCONF);
}

/*
 * what is our OFW node?
 */
int
ebus_find_node(sc, pa)
	struct ebus_softc *sc;
	struct pci_attach_args *pa;
{
	int node = pa->pa_pc->node;
	int n, *ap;
	int pcibus, bus, dev, fn;

	DPRINTF(EDB_PROM, ("ebus_find_node: looking at pci node %08x\n", node));
	for (node = firstchild(node); node; node = nextsibling(node)) {
		char *name = getpropstring(node, "name");

		DPRINTF(EDB_PROM, ("ebus_find_node: looking at PCI device `%s', node = %08x\n", name, node));
		/* must be "ebus" */
		if (strcmp(name, "ebus") != 0)
			continue;

		/* pull the PCI bus out of the pa_tag */
		pcibus = (pa->pa_tag >> 16) & 0xff;
		DPRINTF(EDB_PROM, ("; pcibus %d dev %d fn %d\n", pcibus, pa->pa_device, pa->pa_function));

		/* get the PCI bus/device/function for this node */
		if (getprop(node, "reg", sizeof(int), &n,
		    (void **)&ap))
			continue;

		bus = (ap[0] >> 16) & 0xff;
		dev = (ap[0] >> 11) & 0x1f;
		fn = (ap[0] >> 8) & 0x7;

		DPRINTF(EDB_PROM, ("; looking for bus %d dev %d fn %d\n", pcibus, pa->pa_device, pa->pa_function));
		if (pa->pa_device != dev ||
		    pa->pa_function != fn ||
		    pcibus != bus)
			continue;

		DPRINTF(EDB_PROM, ("; found it, returning %08x\n", node));
		/* found it! */
		free(ap, M_DEVBUF);
		return (node);
	}

	/* damn! */
	return (0);
}
