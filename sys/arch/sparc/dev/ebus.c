/*	$NetBSD: ebus.c,v 1.17 2004/04/24 15:49:00 kleink Exp $ */ 

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

/*
 * EBus support for PCI based SPARC systems (ms-IIep, Ultra).
 * EBus is documented in PCIO manual (Sun Part#: 802-7837-01).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ebus.c,v 1.17 2004/04/24 15:49:00 kleink Exp $");

#if defined(DEBUG) && !defined(EBUS_DEBUG)
#define EBUS_DEBUG
#endif

#ifdef EBUS_DEBUG
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

#include <dev/ofw/ofw_pci.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>


struct ebus_softc {
	struct device			sc_dev;
	struct device			*sc_parent;	/* PCI bus */

	int				sc_node;	/* PROM node */

	bus_space_tag_t			sc_bustag;	/* mem tag from pci */
	bus_dma_tag_t			sc_dmatag;	/* XXX */

	bus_space_tag_t			sc_childbustag;	/* EBus tag */

	/* 
	 * "reg" contains exactly the info we'd get by processing
	 * "ranges", so don't bother with "ranges" and use "reg" directly.
	 */
	struct ofw_pci_register		*sc_reg;
	int				sc_nreg;
};

int	ebus_match(struct device *, struct cfdata *, void *);
void	ebus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ebus, sizeof(struct ebus_softc),
    ebus_match, ebus_attach, NULL, NULL);

int	ebus_setup_attach_args(struct ebus_softc *, int,
			       struct ebus_attach_args *);
void	ebus_destroy_attach_args(struct ebus_attach_args *);
int	ebus_print(void *, const char *);

/*
 * here are our bus space and bus DMA routines.
 */
static paddr_t	ebus_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static int	_ebus_bus_map(bus_space_tag_t, bus_addr_t,
			      bus_size_t, int, vaddr_t, bus_space_handle_t *);
static void	*ebus_intr_establish(bus_space_tag_t, int, int,
				     int (*)(void *), void *, void (*)(void));

static bus_space_tag_t	ebus_alloc_bus_tag(struct ebus_softc *);
static bus_dma_tag_t	ebus_alloc_dma_tag(struct ebus_softc *, bus_dma_tag_t);


/*
 * Working around PROM bogosity.
 *
 * EBus doesn't have official OFW binding.  sparc64 has a de-facto
 * standard but patching it in in prompatch.c and then decoding it
 * here would be an overkill for ms-IIep.
 *
 * So we assume that all ms-IIep based systems use PCIO chip only in
 * "motherboard mode" with interrupt lines wired directly to ms-IIep
 * interrupt inputs.
 *
 * Note that this is ineligible for prompatch.c, as we are not
 * correcting PROM to conform to some established standard, this hack
 * is tied to this version of ebus driver and as such it's better stay
 * private to the driver.
 */

struct msiiep_ebus_intr_wiring {
	const char *name;	/* PROM node */
	int line;		/* ms-IIep interrupt input */
};

static struct msiiep_ebus_intr_wiring krups_ebus_intr_wiring[] = {
	{ "su", 0 }, { "8042", 0 }, { "sound", 3 }
};


struct msiiep_known_ebus_wiring {
	const char *model;
	struct msiiep_ebus_intr_wiring *map;
	int mapsize;
};

#define MSIIEP_MODEL_WIRING(name, map) \
	{ name, map, sizeof(map)/sizeof(map[0]) }

static struct msiiep_known_ebus_wiring known_models[] = {
	MSIIEP_MODEL_WIRING("SUNW,501-4267", krups_ebus_intr_wiring),
	{ NULL, NULL, 0}
};


/*
 * XXX: This assumes single EBus.  However I don't think any ms-IIep
 * system ever used more than one.  In any case, without looking at a
 * system with multiple PCIO chips I don't know how to correctly
 * program the driver to handle PROM glitches in them, so for the time
 * being just use globals.
 */
static struct msiiep_ebus_intr_wiring *wiring_map;
static int wiring_map_size;

static int ebus_init_wiring_table(struct ebus_softc *);


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
	if (node == -1)
		return (0);

	prom_getpropstringA(node, "name", name, sizeof name);
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE
	    && PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SUN
	    && PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SUN_EBUS
	    && strcmp(name, "ebus") == 0)
		return (1);

	return (0);
}


static int
ebus_init_wiring_table(sc)
	struct ebus_softc *sc;
{
	struct msiiep_known_ebus_wiring *p;
	char buf[32];
	char *model;

	if (wiring_map != NULL) {
		printf("%s: global ebus wiring map already initalized\n",
		       sc->sc_dev.dv_xname);
		return (0);
	}

	model = prom_getpropstringA(prom_findroot(), "model",
				    buf, sizeof(buf));
	if (model == NULL)
		panic("ebus_init_wiring_table: no \"model\" property");

	for (p = known_models; p->model != NULL; ++p)
		if (strcmp(model, p->model) == 0) {
			wiring_map = p->map;
			wiring_map_size = p->mapsize;
			return (1);
		}

	/* not found?  we should have failed in pci_attach_hook then. */
	panic("ebus_init_wiring_table: unknown model %s", model);
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
	struct ebus_attach_args ea;
	int node, error;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s, revision 0x%02x\n",
	       devinfo, PCI_REVISION(pa->pa_class));

	node = PCITAG_NODE(pa->pa_tag);
	if (node == -1)
		panic("%s: unable to find ebus node", self->dv_xname);

	if (ebus_init_wiring_table(sc) == 0)
		return;

	sc->sc_node = node;
	sc->sc_parent = parent;	/* XXX: unused so far */
	sc->sc_bustag = pa->pa_memt; /* EBus only does PCI MEM32 space */
	sc->sc_childbustag = ebus_alloc_bus_tag(sc);
	sc->sc_dmatag = ebus_alloc_dma_tag(sc, pa->pa_dmat);

	/*
	 * Setup ranges.  The interesting thing is that we use "reg"
	 * not "ranges", since "reg" on ebus has exactly the data we'd
	 * get by processing "ranges".
	 * 
	 */
	error = prom_getprop(node, "reg", sizeof(struct ofw_pci_register),
			     &sc->sc_nreg, &sc->sc_reg);
	if (error)
		panic("%s: unable to read ebus registers (error %d)",
		      self->dv_xname, error);

	/*
	 * now attach all our children
	 */
	DPRINTF(EDB_CHILD, ("ebus node %08x, searching children...\n", node));
	for (node = firstchild(node); node; node = nextsibling(node)) {
		char *name = prom_getpropstring(node, "name");

		if (ebus_setup_attach_args(sc, node, &ea) != 0) {
			printf("ebus_attach: %s: incomplete\n", name);
			continue;
		}
		DPRINTF(EDB_CHILD,
			("- found child `%s', attaching\n", ea.ea_name));
		(void)config_found(self, &ea, ebus_print);
		ebus_destroy_attach_args(&ea);
	}
}

int
ebus_setup_attach_args(sc, node, ea)
	struct ebus_softc *sc;
	int node;
	struct ebus_attach_args	*ea;
{
	int n, err;

	memset(ea, 0, sizeof(struct ebus_attach_args));

	err = prom_getprop(node, "name", 1, &n, &ea->ea_name);
	if (err != 0)
		return (err);
	ea->ea_name[n] = '\0';

	ea->ea_node = node;
	ea->ea_bustag = sc->sc_childbustag;
	ea->ea_dmatag = sc->sc_dmatag;

	err = prom_getprop(node, "reg", sizeof(struct ebus_regs),
			   &ea->ea_nreg, &ea->ea_reg);
	if (err != 0)
		return (err);

	/*
	 * On Ultra the bar is the _offset_ of the BAR in PCI config
	 * space but in (some?) ms-IIep systems (e.g. Krups) it's the
	 * _number_ of the BAR - e.g. BAR1 is represented by 1 in
	 * Krups PROM, while on Ultra it's 0x14.  Fix it here.
	 */
	for (n = 0; n < ea->ea_nreg; ++n)
	    if (ea->ea_reg[n].hi < PCI_MAPREG_START) {
		ea->ea_reg[n].hi = PCI_MAPREG_START
		    + ea->ea_reg[n].hi * sizeof(pcireg_t);
	    }


	err = prom_getprop(node, "address", sizeof(u_int32_t),
			   &ea->ea_nvaddr, &ea->ea_vaddr);
	if (err != ENOENT) {
		if (err != 0)
			return (err);

		if (ea->ea_nreg != ea->ea_nvaddr)
			printf("ebus loses: device %s: %d regs and %d addrs\n",
			       ea->ea_name, ea->ea_nreg, ea->ea_nvaddr);
	} else
		ea->ea_nvaddr = 0;

	/* XXX: "interrupts" hack */
	for (n = 0; n < wiring_map_size; ++n) {
		struct msiiep_ebus_intr_wiring *w = &wiring_map[n];
		if (strcmp(w->name, ea->ea_name) == 0) {
			ea->ea_intr = malloc(sizeof(u_int32_t),
					     M_DEVBUF, M_NOWAIT);
			ea->ea_intr[0] = w->line;
			ea->ea_nintr = 1;
			break;
		}
	}

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
	for (i = 0; i < ea->ea_nreg; ++i)
		aprint_normal("%s bar %x offset 0x%x", i == 0 ? "" : ",",
		       ea->ea_reg[i].hi, ea->ea_reg[i].lo);
	for (i = 0; i < ea->ea_nintr; ++i)
		aprint_normal(" line %d", ea->ea_intr[i]);
	return (UNCONF);
}


/*
 * bus space and bus DMA methods below here
 */

bus_space_tag_t
ebus_alloc_bus_tag(sc)
	struct ebus_softc *sc;
{
	bus_space_tag_t bt;

	bt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (bt == NULL)
		panic("unable to allocate ebus bus tag");

	memset(bt, 0, sizeof *bt);
	bt->cookie = sc;
	bt->parent = sc->sc_bustag;
	bt->sparc_bus_map = _ebus_bus_map;
	bt->sparc_bus_mmap = ebus_bus_mmap;
	bt->sparc_intr_establish = ebus_intr_establish;
	return (bt);
}


bus_dma_tag_t
ebus_alloc_dma_tag(sc, pdt)
	struct ebus_softc *sc;
	bus_dma_tag_t pdt;
{
	bus_dma_tag_t dt;

	dt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (dt == NULL)
		panic("unable to allocate ebus DMA tag");

	memset(dt, 0, sizeof *dt);
	dt->_cookie = sc;
#define PCOPY(x)	dt->x = pdt->x
	PCOPY(_dmamap_create);
	PCOPY(_dmamap_destroy);
	PCOPY(_dmamap_load);
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	PCOPY(_dmamap_load_raw);
	PCOPY(_dmamap_unload);
	PCOPY(_dmamap_sync);
	PCOPY(_dmamem_alloc);
	PCOPY(_dmamem_free);
	PCOPY(_dmamem_map);
	PCOPY(_dmamem_unmap);
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	return (dt);
}

/*
 * bus space support.  <sparc64/dev/psychoreg.h> has a discussion
 * about PCI physical addresses, which also applies to ebus.
 */
static int
_ebus_bus_map(t, ba, size, flags, va, hp)
	bus_space_tag_t t;
	bus_addr_t ba;	/* encodes bar/offset */
	bus_size_t size;
	int	flags;
	vaddr_t va;
	bus_space_handle_t *hp;
{
	struct ebus_softc *sc = t->cookie;
	u_int bar;
	paddr_t offset;
	int i;

	bar = BUS_ADDR_IOSPACE(ba);
	offset = BUS_ADDR_PADDR(ba);

	DPRINTF(EDB_BUSMAP,
		("\n_ebus_bus_map: bar %d offset %08x sz %x flags %x va %p\n",
		 (int)bar, (u_int32_t)offset, (u_int32_t)size,
		 flags, (void *)va));

	/* EBus has only two BARs */
	if (PCI_MAPREG_NUM(bar) > 1) {
		DPRINTF(EDB_BUSMAP,
			("\n_ebus_bus_map: impossible bar\n"));
		return (EINVAL);
	}

	/*
	 * Almost all of the interesting ebus children are mapped by
	 * BAR1, the last entry in sc_reg[], so work our way backwards.
	 */
	for (i = sc->sc_nreg - 1; i >= 0; --i) {
		bus_addr_t pciaddr;
		u_int32_t ss;

		/* EBus only does MEM32 */
		ss  = sc->sc_reg[i].phys_hi & OFW_PCI_PHYS_HI_SPACEMASK;
		if (ss != OFW_PCI_PHYS_HI_SPACE_MEM32)
			continue;

		if (bar != (sc->sc_reg[i].phys_hi
			    & OFW_PCI_PHYS_HI_REGISTERMASK))
			continue;

		pciaddr = (bus_addr_t)sc->sc_reg[i].phys_lo + offset;

		if (pciaddr + size > sc->sc_reg[i].phys_lo
					+ sc->sc_reg[i].size_lo)
			continue;

		DPRINTF(EDB_BUSMAP,
			("_ebus_bus_map: mapping to PCI addr %x\n",
			 (u_int32_t)pciaddr));

		/* pass it onto the pci controller */
		return (bus_space_map2(sc->sc_bustag, pciaddr, size,
				       flags, va, hp));
	}

	DPRINTF(EDB_BUSMAP, (": FAILED\n"));
	return (EINVAL);
}

static paddr_t
ebus_bus_mmap(t, ba, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t ba;
	off_t off;
	int prot;
	int flags;
{

	/* XXX: not implemetned yet */
	return (-1);
}

/* 
 * Install an interrupt handler for a EBus device.
 */
void *
ebus_intr_establish(t, pri, level, handler, arg, fastvec)
	bus_space_tag_t t;
	int pri;
	int level;
	int (*handler)(void *);
	void *arg;
	void (*fastvec)(void);	/* ignored */
{
	return (bus_intr_establish(t->parent, pri, level, handler, arg));
}
