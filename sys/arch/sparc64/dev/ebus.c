/*	$NetBSD: ebus.c,v 1.14 2000/06/29 07:37:54 mrg Exp $	*/

/*
 * Copyright (c) 1999, 2000 Matthew R. Green
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

#include "opt_ddb.h"

/*
 * UltraSPARC 5 and beyond ebus support.
 *
 * note that this driver is far from complete:
 *	- ebus2 dma code is completely unwritten
 *	- interrupt establish code is completely unwritten
 *	- bus map code is written and appears to work
 */

#undef DEBUG
#define DEBUG

#ifdef DEBUG
#define	EDB_PROM	0x01
#define EDB_CHILD	0x02
#define	EDB_INTRMAP	0x04
#define EDB_BUSMAP	0x08
#define EDB_BUSDMA	0x10
#define EDB_INTR	0x20
int ebus_debug = 0;
#define DPRINTF(l, s)   do { if (ebus_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/time.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <sparc64/dev/ebusreg.h>
#include <sparc64/dev/ebusvar.h>
#include <sparc64/sparc64/cache.h>

int	ebus_match __P((struct device *, struct cfdata *, void *));
void	ebus_attach __P((struct device *, struct device *, void *));

struct cfattach ebus_ca = {
	sizeof(struct ebus_softc), ebus_match, ebus_attach
};

int	ebus_setup_attach_args __P((struct ebus_softc *, int,
	    struct ebus_attach_args *));
void	ebus_destroy_attach_args __P((struct ebus_attach_args *));
int	ebus_print __P((void *, const char *));
void	ebus_find_ino __P((struct ebus_softc *, struct ebus_attach_args *));
int	ebus_find_node __P((struct pci_attach_args *));

/*
 * here are our bus space and bus dma routines.
 */
static int ebus_bus_mmap __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				int, bus_space_handle_t *));
static int _ebus_bus_map __P((bus_space_tag_t, bus_type_t, bus_addr_t,
				bus_size_t, int, vaddr_t,
				bus_space_handle_t *));
static void *ebus_intr_establish __P((bus_space_tag_t, int, int,
				int (*) __P((void *)), void *));

static int ebus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
			  bus_size_t, struct proc *, int));
static void ebus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void ebus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
				  bus_size_t, int));
int ebus_dmamem_alloc __P((bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t,
			   bus_dma_segment_t *, int, int *, int));
void ebus_dmamem_free __P((bus_dma_tag_t, bus_dma_segment_t *, int));
int ebus_dmamem_map __P((bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
			 caddr_t *, int));
void ebus_dmamem_unmap __P((bus_dma_tag_t, caddr_t, size_t));

int
ebus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN && 
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_EBUS &&
	    ebus_find_node(pa))
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

	sc->sc_parent = (struct psycho_softc *)parent;
	sc->sc_bustag = pa->pa_memt;
	sc->sc_childbustag = ebus_alloc_bus_tag(sc, PCI_MEMORY_BUS_SPACE);
	sc->sc_dmatag = ebus_alloc_dma_tag(sc, pa->pa_dmat);

	node = ebus_find_node(pa);
	if (node == 0)
		panic("could not find ebus node");

	sc->sc_node = node;

	/*
	 * fill in our softc with information from the prom
	 */
	sc->sc_intmap = NULL;
	sc->sc_range = NULL;
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
		} else {
			DPRINTF(EDB_CHILD, ("- found child `%s', attaching\n", eba.ea_name));
			(void)config_found(self, &eba, ebus_print);
		}
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

	rv = getprop(node, "address", sizeof(u_int32_t), &ea->ea_nvaddrs,
	    (void **)&ea->ea_vaddrs);
	if (rv != ENOENT) {
		if (rv)
			return (rv);

		if (ea->ea_nregs != ea->ea_nvaddrs)
			printf("ebus loses: device %s: %d regs and %d addrs\n",
			    ea->ea_name, ea->ea_nregs, ea->ea_nvaddrs);
	} else
		ea->ea_nvaddrs = 0;

	if (getprop(node, "interrupts", sizeof(u_int32_t), &ea->ea_nintrs,
	    (void **)&ea->ea_intrs))
		ea->ea_nintrs = 0;
	else 
		ebus_find_ino(sc, ea);

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
	int i;

	if (p)
		printf("%s at %s", ea->ea_name, p);
	for (i = 0; i < ea->ea_nregs; i++)
		printf(" addr %x-%x", ea->ea_regs[i].lo,
		    ea->ea_regs[i].lo + ea->ea_regs[i].size - 1);
	for (i = 0; i < ea->ea_nintrs; i++)
		printf(" ipl %d", ea->ea_intrs[i]);
	return (UNCONF);
}

/*
 * find the INO values for each interrupt and fill them in.
 *
 * for each "reg" property of this device, mask it's hi and lo
 * values with the "interrupt-map-mask"'s hi/lo values, and also
 * mask the interrupt number with the interrupt mask.  search the
 * "interrupt-map" list for matching values of hi, lo and interrupt
 * to give the INO for this interrupt.
 */
void
ebus_find_ino(sc, ea)
	struct ebus_softc *sc;
	struct ebus_attach_args *ea;
{
	u_int32_t hi, lo, intr;
	int i, j, k;

	DPRINTF(EDB_INTRMAP, ("ebus_find_ino: searching %d interrupts", ea->ea_nintrs));
	for (j = 0; j < ea->ea_nintrs; j++) {
		intr = ea->ea_intrs[j] & sc->sc_intmapmask.intr;

		DPRINTF(EDB_INTRMAP, ("; intr %x masked to %x", ea->ea_intrs[j], intr));
		for (i = 0; i < ea->ea_nregs; i++) {
			hi = ea->ea_regs[i].hi & sc->sc_intmapmask.hi;
			lo = ea->ea_regs[i].lo & sc->sc_intmapmask.lo;

			DPRINTF(EDB_INTRMAP, ("; reg hi.lo %08x.08x masked to %08x.%08x", ea->ea_regs[i].hi, ea->ea_regs[i].lo, hi, lo));
			for (k = 0; k < sc->sc_nintmap; k++) {
				DPRINTF(EDB_INTRMAP, ("; checking hi.lo %08x.%08x intr %x", sc->sc_intmap[k].hi, sc->sc_intmap[k].lo, sc->sc_intmap[k].intr));
				if (hi == sc->sc_intmap[k].hi &&
				    lo == sc->sc_intmap[k].lo &&
				    intr == sc->sc_intmap[k].intr) {
					ea->ea_intrs[j] = 
						sc->sc_intmap[k].cintr|INTMAP_OBIO;
					DPRINTF(EDB_INTRMAP, ("; FOUND IT! changing to %d\n", sc->sc_intmap[k].cintr));
					goto next_intr;
				}
			}
		}
next_intr:
	}
}

/*
 * what is our OFW node?  this depends on our pci chipset tag
 * having it's "node" value set to the OFW node of the PCI bus,
 * see the simba driver.
 */
int
ebus_find_node(pa)
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
		ap = NULL;
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

/*
 * bus space and bus dma below here
 */
bus_space_tag_t
ebus_alloc_bus_tag(sc, type)
	struct ebus_softc *sc;
	int type;
{
	bus_space_tag_t bt;

	bt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (bt == NULL)
		panic("could not allocate ebus bus tag");

	bzero(bt, sizeof *bt);
	bt->cookie = sc;
	bt->parent = sc->sc_bustag;
	bt->type = type;
	bt->sparc_bus_map = _ebus_bus_map;
	bt->sparc_bus_mmap = ebus_bus_mmap;
	bt->sparc_intr_establish = ebus_intr_establish;
	return (bt);
}

/* XXX? */
bus_dma_tag_t
ebus_alloc_dma_tag(sc, pdt)
	struct ebus_softc *sc;
	bus_dma_tag_t pdt;
{
	bus_dma_tag_t dt;

	dt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (dt == NULL)
		panic("could not allocate ebus dma tag");

	bzero(dt, sizeof *dt);
	dt->_cookie = sc;
	dt->_parent = pdt;
#define PCOPY(x)	dt->x = pdt->x
	PCOPY(_dmamap_create);
	PCOPY(_dmamap_destroy);
	dt->_dmamap_load = ebus_dmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	PCOPY(_dmamap_load_raw);
	dt->_dmamap_unload = ebus_dmamap_unload;
	dt->_dmamap_sync = ebus_dmamap_sync;
	dt->_dmamem_alloc = ebus_dmamem_alloc;
	dt->_dmamem_free = ebus_dmamem_free;
	dt->_dmamem_map = ebus_dmamem_map;
	dt->_dmamem_unmap = ebus_dmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion
 * about PCI physical addresses, which also applies to ebus.
 */
static int
_ebus_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct ebus_softc *sc = t->cookie;
	bus_addr_t hi, lo;
	int i;

	DPRINTF(EDB_BUSMAP, ("\n_ebus_bus_map: type %d off %016llx sz %x flags %d va %p", (int)t->type, (u_int64_t)offset, (int)size, (int)flags, vaddr));

	hi = offset >> 32UL;
	lo = offset & 0xffffffff;
	DPRINTF(EDB_BUSMAP, (" (hi %08x lo %08x)", (u_int)hi, (u_int)lo));
	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t pciaddr;

		if (hi != sc->sc_range[i].child_hi)
			continue;
		if (lo < sc->sc_range[i].child_lo ||
		    (lo + size) > (sc->sc_range[i].child_lo + sc->sc_range[i].size))
			continue;

		pciaddr = ((bus_addr_t)sc->sc_range[i].phys_mid << 32UL) |
				       sc->sc_range[i].phys_lo;
		pciaddr += lo;
		DPRINTF(EDB_BUSMAP, ("\n_ebus_bus_map: mapping paddr offset %qx pciaddr %qx\n",
			       offset, pciaddr));
		/* pass it onto the psycho */
		return (bus_space_map2(sc->sc_bustag, t->type, pciaddr,
					size, flags, vaddr, hp));
	}
	DPRINTF(EDB_BUSMAP, (": FAILED\n"));
	return (EINVAL);
}

static int
ebus_bus_mmap(t, btype, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t paddr;
	int flags;
	bus_space_handle_t *hp;
{
	bus_addr_t offset = paddr;
	struct ebus_softc *sc = t->cookie;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr = ((bus_addr_t)sc->sc_range[i].child_hi << 32) |
		    sc->sc_range[i].child_lo;

		if (offset != paddr)
			continue;

		DPRINTF(EDB_BUSMAP, ("\n_ebus_bus_mmap: mapping paddr %qx\n", paddr));
		return (bus_space_mmap(sc->sc_bustag, 0, paddr,
				       flags, hp));
	}

	return (-1);
}

/*
 * install an interrupt handler for a PCI device
 */
void *
ebus_intr_establish(t, level, flags, handler, arg)
	bus_space_tag_t t;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	return (bus_intr_establish(t->parent, level, flags, handler, arg));
}

/*
 * bus dma support
 */
int
ebus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct ebus_softc *sc = t->_cookie;

	return (iommu_dvmamap_load(t, &sc->sc_parent->sc_is, map, buf, buflen,
	    p, flags));
}

void
ebus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct ebus_softc *sc = t->_cookie;

	iommu_dvmamap_unload(t, &sc->sc_parent->sc_is, map);
}

void
ebus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	struct ebus_softc *sc = t->_cookie;

	iommu_dvmamap_sync(t, &sc->sc_parent->sc_is, map, offset, len, ops);
	bus_dmamap_sync(t->_parent, map, offset, len, ops);
}

int
ebus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size;
	bus_size_t alignment;
	bus_size_t boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	struct ebus_softc *sc = t->_cookie;

	return (iommu_dvmamem_alloc(t, &sc->sc_parent->sc_is, size, alignment,
	    boundary, segs, nsegs, rsegs, flags));
}

void
ebus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct ebus_softc *sc = t->_cookie;

	iommu_dvmamem_free(t, &sc->sc_parent->sc_is, segs, nsegs);
}

int
ebus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct ebus_softc *sc = t->_cookie;

	return (iommu_dvmamem_map(t, &sc->sc_parent->sc_is, segs, nsegs,
	    size, kvap, flags));
}

void
ebus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{
	struct ebus_softc *sc = t->_cookie;

	iommu_dvmamem_unmap(t, &sc->sc_parent->sc_is, kva, size);
}
