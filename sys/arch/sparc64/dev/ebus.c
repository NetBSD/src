/*	$NetBSD: ebus.c,v 1.39 2003/05/17 01:38:39 nakayama Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001 Matthew R. Green
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
 * note that this driver is not complete:
 *	- interrupt establish is written and appears to work
 *	- bus map code is written and appears to work
 *	- ebus2 DMA code is completely unwritten, we just punt to
 *	  the iommu.
 */

#ifdef DEBUG
#define	EDB_PROM	0x01
#define EDB_CHILD	0x02
#define	EDB_INTRMAP	0x04
#define EDB_BUSMAP	0x08
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
#include <machine/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/psychoreg.h>
#include <sparc64/dev/psychovar.h>
#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <sparc64/sparc64/cache.h>

struct ebus_softc {
	struct device			sc_dev;

	int				sc_node;

	bus_space_tag_t			sc_memtag;	/* from pci */
	bus_space_tag_t			sc_iotag;	/* from pci */
	bus_space_tag_t			sc_childbustag;	/* pass to children */
	bus_dma_tag_t			sc_dmatag;

	struct ebus_ranges		*sc_range;
	struct ebus_interrupt_map	*sc_intmap;
	struct ebus_interrupt_map_mask	sc_intmapmask;

	int				sc_nrange;	/* counters */
	int				sc_nintmap;
};

int	ebus_match __P((struct device *, struct cfdata *, void *));
void	ebus_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(ebus, sizeof(struct ebus_softc),
    ebus_match, ebus_attach, NULL, NULL);

bus_space_tag_t ebus_alloc_bus_tag __P((struct ebus_softc *, int));

int	ebus_setup_attach_args __P((struct ebus_softc *, int,
	    struct ebus_attach_args *));
void	ebus_destroy_attach_args __P((struct ebus_attach_args *));
int	ebus_print __P((void *, const char *));
void	ebus_find_ino __P((struct ebus_softc *, struct ebus_attach_args *));
int	ebus_find_node __P((struct pci_attach_args *));

/*
 * here are our bus space and bus DMA routines.
 */
static paddr_t ebus_bus_mmap __P((bus_space_tag_t, bus_addr_t, off_t, int, int));
static int _ebus_bus_map __P((bus_space_tag_t, bus_addr_t, bus_size_t, int, 
			      vaddr_t, bus_space_handle_t *));
static void *ebus_intr_establish __P((bus_space_tag_t, int, int,
				int (*) __P((void *)), void *, void(*)__P((void))));

int
ebus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char name[10];
	int node;

	/* Only attach if there's a PROM node. */
	node = PCITAG_NODE(pa->pa_tag);
	if (node == -1) return (0);

	/* Match a real ebus */
	OF_getprop(node, "name", &name, sizeof(name));
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_EBUS &&
		strcmp(name, "ebus") == 0)
		return (1);

	/* Or a real ebus III */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_EBUSIII &&
		strcmp(name, "ebus") == 0)
		return (1);

	/* Or a PCI-ISA bridge XXX I hope this is on-board. */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_ISA) {
		return (1);
	}

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
	int node, nmapmask, error;
	char devinfo[256];

	printf("\n");

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s, revision 0x%02x\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	sc->sc_memtag = pa->pa_memt;
	sc->sc_iotag = pa->pa_iot;
	sc->sc_childbustag = ebus_alloc_bus_tag(sc, PCI_MEMORY_BUS_SPACE);
	sc->sc_dmatag = pa->pa_dmat;

	node = PCITAG_NODE(pa->pa_tag);
	if (node == -1)
		panic("could not find ebus node");

	sc->sc_node = node;

	/*
	 * fill in our softc with information from the prom
	 */
	sc->sc_intmap = NULL;
	sc->sc_range = NULL;
	error = PROM_getprop(node, "interrupt-map",
			sizeof(struct ebus_interrupt_map),
			&sc->sc_nintmap, (void **)&sc->sc_intmap);
	switch (error) {
	case 0:
		immp = &sc->sc_intmapmask;
		error = PROM_getprop(node, "interrupt-map-mask",
			    sizeof(struct ebus_interrupt_map_mask), &nmapmask,
			    (void *)&immp);
		if (error)
			panic("could not get ebus interrupt-map-mask");
		if (nmapmask != 1)
			panic("ebus interrupt-map-mask is broken");
		break;
	case ENOENT:
		break;
	default:
		panic("ebus interrupt-map: error %d", error);
		break;
	}

	error = PROM_getprop(node, "ranges", sizeof(struct ebus_ranges),
	    &sc->sc_nrange, (void **)&sc->sc_range);
	if (error)
		panic("ebus ranges: error %d", error);

	/*
	 * now attach all our children
	 */
	DPRINTF(EDB_CHILD, ("ebus node %08x, searching children...\n", node));
	for (node = firstchild(node); node; node = nextsibling(node)) {
		char *name = PROM_getpropstring(node, "name");

		if (ebus_setup_attach_args(sc, node, &eba) != 0) {
			printf("ebus_attach: %s: incomplete\n", name);
			continue;
		} else {
			DPRINTF(EDB_CHILD, ("- found child `%s', attaching\n",
			    eba.ea_name));
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
	rv = PROM_getprop(node, "name", 1, &n, (void **)&ea->ea_name);
	if (rv != 0)
		return (rv);
	ea->ea_name[n] = '\0';

	ea->ea_node = node;
	ea->ea_bustag = sc->sc_childbustag;
	ea->ea_dmatag = sc->sc_dmatag;

	rv = PROM_getprop(node, "reg", sizeof(struct ebus_regs), &ea->ea_nreg,
	    (void **)&ea->ea_reg);
	if (rv)
		return (rv);

	rv = PROM_getprop(node, "address", sizeof(u_int32_t), &ea->ea_nvaddr,
	    (void **)&ea->ea_vaddr);
	if (rv != ENOENT) {
		if (rv)
			return (rv);

		if (ea->ea_nreg != ea->ea_nvaddr)
			printf("ebus loses: device %s: %d regs and %d addrs\n",
			    ea->ea_name, ea->ea_nreg, ea->ea_nvaddr);
	} else
		ea->ea_nvaddr = 0;

	if (PROM_getprop(node, "interrupts", sizeof(u_int32_t), &ea->ea_nintr,
	    (void **)&ea->ea_intr))
		ea->ea_nintr = 0;
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
	if (ea->ea_reg)
		free((void *)ea->ea_reg, M_DEVBUF);
	if (ea->ea_intr)
		free((void *)ea->ea_intr, M_DEVBUF);
	if (ea->ea_vaddr)
		free((void *)ea->ea_vaddr, M_DEVBUF);
}

int
ebus_print(aux, p)
	void *aux;
	const char *p;
{
	struct ebus_attach_args *ea = aux;
	int i;

	if (p)
		aprint_normal("%s at %s", ea->ea_name, p);
	for (i = 0; i < ea->ea_nreg; i++)
		aprint_normal("%s %x-%x", i == 0 ? " addr" : ",",
		    ea->ea_reg[i].lo,
		    ea->ea_reg[i].lo + ea->ea_reg[i].size - 1);
	for (i = 0; i < ea->ea_nintr; i++)
		aprint_normal(" ipl %d", ea->ea_intr[i]);
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

	if (sc->sc_nintmap == 0) {
		for (i = 0; i < ea->ea_nintr; i++) {
			OF_mapintr(ea->ea_node, &ea->ea_intr[i],
				sizeof(ea->ea_intr[0]),
				sizeof(ea->ea_intr[0]));
		}
		return;
	}

	DPRINTF(EDB_INTRMAP,
	    ("ebus_find_ino: searching %d interrupts", ea->ea_nintr));

	for (j = 0; j < ea->ea_nintr; j++) {

		intr = ea->ea_intr[j] & sc->sc_intmapmask.intr;

		DPRINTF(EDB_INTRMAP,
		    ("; intr %x masked to %x", ea->ea_intr[j], intr));
		for (i = 0; i < ea->ea_nreg; i++) {
			hi = ea->ea_reg[i].hi & sc->sc_intmapmask.hi;
			lo = ea->ea_reg[i].lo & sc->sc_intmapmask.lo;

			DPRINTF(EDB_INTRMAP,
			    ("; reg hi.lo %08x.%08x masked to %08x.%08x",
			    ea->ea_reg[i].hi, ea->ea_reg[i].lo, hi, lo));
			for (k = 0; k < sc->sc_nintmap; k++) {
				DPRINTF(EDB_INTRMAP,
				    ("; checking hi.lo %08x.%08x intr %x",
				    sc->sc_intmap[k].hi, sc->sc_intmap[k].lo,
				    sc->sc_intmap[k].intr));
				if (hi == sc->sc_intmap[k].hi &&
				    lo == sc->sc_intmap[k].lo &&
				    intr == sc->sc_intmap[k].intr) {
					ea->ea_intr[j] =
					    sc->sc_intmap[k].cintr;
					DPRINTF(EDB_INTRMAP,
					    ("; FOUND IT! changing to %d\n",
					    sc->sc_intmap[k].cintr));
					goto next_intr;
				}
			}
		}
next_intr:;
	}
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion
 * about PCI physical addresses, which also applies to ebus.
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
	bt->parent = sc->sc_memtag;
	bt->type = type;
	bt->sparc_bus_map = _ebus_bus_map;
	bt->sparc_bus_mmap = ebus_bus_mmap;
	bt->sparc_intr_establish = ebus_intr_establish;
	return (bt);
}

static int
_ebus_bus_map(t, ba, size, flags, va, hp)
	bus_space_tag_t t;
	bus_addr_t ba;
	bus_size_t size;
	int	flags;
	vaddr_t va;
	bus_space_handle_t *hp;
{
	struct ebus_softc *sc = t->cookie;
	paddr_t offset;
	u_int bar;
	int i, ss;

	bar = BUS_ADDR_IOSPACE(ba);
	offset = BUS_ADDR_PADDR(ba);

	DPRINTF(EDB_BUSMAP,
		("\n_ebus_bus_map: bar %d offset %08x sz %x flags %x va %p\n",
		 (int)bar, (u_int32_t)offset, (u_int32_t)size,
		 flags, (void *)va));

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t pciaddr;

		if (bar != sc->sc_range[i].child_hi)
			continue;
		if (offset < sc->sc_range[i].child_lo ||
		    (offset + size) >
		      (sc->sc_range[i].child_lo + sc->sc_range[i].size))
			continue;

		/* Isolate address space and find the right tag */
		ss = (sc->sc_range[i].phys_hi>>24)&3;
		switch (ss) {
		case 1:	/* I/O space */
			t = sc->sc_iotag;
			break;
		case 2:	/* Memory space */
			t = sc->sc_memtag;
			break;
		case 0:	/* Config space */
		case 3:	/* 64-bit Memory space */
		default: /* WTF? */
			/* We don't handle these */
			panic("_ebus_bus_map: illegal space %x", ss);
			break;
		}
		pciaddr = ((bus_addr_t)sc->sc_range[i].phys_mid << 32UL) |
				       sc->sc_range[i].phys_lo;
		pciaddr += offset;

		DPRINTF(EDB_BUSMAP,
			("_ebus_bus_map: mapping to PCI addr %x\n",
			 (u_int32_t)pciaddr));

		/* pass it onto the psycho */
		return (bus_space_map(t, pciaddr, size, flags, hp));
	}
	DPRINTF(EDB_BUSMAP, (": FAILED\n"));
	return (EINVAL);
}

static paddr_t
ebus_bus_mmap(t, paddr, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t paddr;
	off_t off;
	int prot;
	int flags;
{
	bus_addr_t offset = paddr;
	struct ebus_softc *sc = t->cookie;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr = ((bus_addr_t)sc->sc_range[i].child_hi << 32) |
		    sc->sc_range[i].child_lo;

		if (offset != paddr)
			continue;

		DPRINTF(EDB_BUSMAP, ("\n_ebus_bus_mmap: mapping paddr %qx\n",
		    (unsigned long long)paddr));
		return (bus_space_mmap(sc->sc_memtag, paddr, off,
				       prot, flags));
	}

	return (-1);
}

/*
 * install an interrupt handler for a ebus device
 */
void *
ebus_intr_establish(t, pri, level, handler, arg, fastvec)
	bus_space_tag_t t;
	int pri;
	int level;
	int (*handler) __P((void *));
	void *arg;
	void (*fastvec) __P((void));	/* ignored */
{

	return (bus_intr_establish(t->parent, pri, level, handler, arg));
}
