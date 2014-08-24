/*	$NetBSD: ebus_mainbus.c,v 1.13 2014/08/24 19:06:14 palle Exp $	*/
/*	$OpenBSD: ebus_mainbus.c,v 1.7 2010/11/11 17:58:23 miod Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ebus_mainbus.c,v 1.13 2014/08/24 19:06:14 palle Exp $");

#ifdef DEBUG
#define	EDB_PROM	0x01
#define EDB_CHILD	0x02
#define	EDB_INTRMAP	0x04
#define EDB_BUSMAP	0x08
#define EDB_BUSDMA	0x10
#define EDB_INTR	0x20
extern int ebus_debug;
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
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/pci/pcivar.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/pyrovar.h>
#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <sparc64/dev/ebusvar.h>

extern struct cfdriver pyro_cd;

int	ebus_mainbus_match(device_t, cfdata_t, void *);
void	ebus_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ebus_mainbus, sizeof(struct ebus_softc),
    ebus_mainbus_match, ebus_mainbus_attach, NULL, NULL);

static int ebus_mainbus_bus_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	vaddr_t, bus_space_handle_t *);
static void *ebus_mainbus_intr_establish(bus_space_tag_t, int, int,
	int (*)(void *), void *, void (*)(void));
static bus_space_tag_t ebus_mainbus_alloc_bus_tag(struct ebus_softc *,
	bus_space_tag_t, int);
#ifdef SUN4V
#if 0
XXX
static void ebus_mainbus_intr_ack(struct intrhand *);
#endif
#endif
int
ebus_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "ebus") == 0)
		return (1);
	return (0);
}

void
ebus_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct ebus_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	struct ebus_attach_args eba;
	struct ebus_interrupt_map_mask *immp;
	int node, nmapmask, error;
	struct pyro_softc *psc;
	int i, j;

	sc->sc_dev = self;
	
	printf("\n");

	sc->sc_memtag = ebus_mainbus_alloc_bus_tag(sc, ma->ma_bustag,
						   PCI_MEMORY_BUS_SPACE);
	sc->sc_iotag = ebus_mainbus_alloc_bus_tag(sc, ma->ma_bustag,
						  PCI_IO_BUS_SPACE);
	sc->sc_childbustag = sc->sc_memtag;
	sc->sc_dmatag = ma->ma_dmatag;

	sc->sc_node = node = ma->ma_node;
	
	/*
	 * fill in our softc with information from the prom
	 */
	sc->sc_intmap = NULL;
	sc->sc_range = NULL;
	error = prom_getprop(node, "interrupt-map",
			sizeof(struct ebus_interrupt_map),
			&sc->sc_nintmap, (void **)&sc->sc_intmap);
	switch (error) {
	case 0:
		immp = &sc->sc_intmapmask;
		nmapmask = 1;
		error = prom_getprop(node, "interrupt-map-mask",
			    sizeof(struct ebus_interrupt_map_mask), &nmapmask,
			    (void **)&immp);
		if (error)
			panic("could not get ebus interrupt-map-mask: error %d",
			      error);
		if (nmapmask != 1)
			panic("ebus interrupt-map-mask is broken");
		break;
	case ENOENT:
		break;
	default:
		panic("ebus interrupt-map: error %d", error);
		break;
	}

	/*
	 * Ebus interrupts may be connected to any of the PCI Express
	 * leafs.  Here we add the appropriate IGN to the interrupt
	 * mappings such that we can use it to distingish between
	 * interrupts connected to PCIE-A and PCIE-B.
	 */
	for (i = 0; i < sc->sc_nintmap; i++) {
		for (j = 0; j < pyro_cd.cd_ndevs; j++) {
			device_t dt = pyro_cd.cd_devs[j];
			psc = device_private(dt);
			if (psc && psc->sc_node == sc->sc_intmap[i].cnode) {
				sc->sc_intmap[i].cintr |= psc->sc_ign;
				break;
			}
		}
	}
	
	error = prom_getprop(node, "ranges", sizeof(struct ebus_mainbus_ranges),
	    &sc->sc_nrange, (void **)&sc->sc_range);
	if (error)
		panic("ebus ranges: error %d", error);

	/*
	 * now attach all our children
	 */
	DPRINTF(EDB_CHILD, ("ebus node %08x, searching children...\n", node));
	for (node = firstchild(node); node; node = nextsibling(node)) {
		if (ebus_setup_attach_args(sc, node, &eba) != 0) {
			DPRINTF(EDB_CHILD,
			    ("ebus_mainbus_attach: %s: incomplete\n",
			    prom_getpropstring(node, "name")));
			continue;
		} else {
			DPRINTF(EDB_CHILD, ("- found child `%s', attaching\n",
			    eba.ea_name));
			(void)config_found(self, &eba, ebus_print);
		}
		ebus_destroy_attach_args(&eba);
	}
}

static bus_space_tag_t
ebus_mainbus_alloc_bus_tag(struct ebus_softc *sc,
			   bus_space_tag_t parent,
			   int type)
{
	struct sparc_bus_space_tag *bt;

	bt = malloc(sizeof(*bt), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (bt == NULL)
		panic("could not allocate ebus bus tag");

	bt->cookie = sc;
	bt->parent = parent;
	bt->type = type;
	bt->sparc_bus_map = ebus_mainbus_bus_map;
	bt->sparc_bus_mmap = ebus_bus_mmap;
	bt->sparc_intr_establish = ebus_mainbus_intr_establish;

	return (bt);
}

int
ebus_mainbus_bus_map(bus_space_tag_t t, bus_addr_t offset, bus_size_t size,
	int flags, vaddr_t va, bus_space_handle_t *hp)
{
	struct ebus_softc *sc = t->cookie;
	struct ebus_mainbus_ranges *range;
	bus_addr_t hi, lo;
	int i;
#if 0
	int ss;
#endif

	DPRINTF(EDB_BUSMAP,
	    ("\n_ebus_mainbus_bus_map: off %016llx sz %x flags %d",
	    (unsigned long long)offset, (int)size, (int)flags));

	if (t->parent == 0 || t->parent->sparc_bus_map == 0) {
		printf("\n_ebus_mainbus_bus_map: invalid parent");
		return (EINVAL);
	}

	t = t->parent;

	hi = offset >> 32UL;
	lo = offset & 0xffffffff;
	range = (struct ebus_mainbus_ranges *)sc->sc_range;

	DPRINTF(EDB_BUSMAP, (" (hi %08x lo %08x)", (u_int)hi, (u_int)lo));
	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t addr;

		if (hi != range[i].child_hi)
			continue;
		if (lo < range[i].child_lo ||
		    (lo + size) > (range[i].child_lo + range[i].size))
			continue;

#if 0
		/* Isolate address space and find the right tag */
		ss = (range[i].phys_hi>>24)&3;
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
			panic("ebus_mainbus_bus_map: illegal space %x", ss);
			break;
		}
#endif

		addr = ((bus_addr_t)range[i].phys_hi << 32UL) |
				    range[i].phys_lo;
		addr += lo;
		DPRINTF(EDB_BUSMAP,
		    ("\n_ebus_mainbus_bus_map: paddr offset %qx addr %qx\n", 
		    (unsigned long long)offset, (unsigned long long)addr));
		return (bus_space_map(t, addr, size, flags, hp));
	}
	DPRINTF(EDB_BUSMAP, (": FAILED\n"));
	return (EINVAL);
}

static void *
ebus_mainbus_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct intrhand *ih = NULL;
	volatile u_int64_t *intrmapptr = NULL, *intrclrptr = NULL;
	u_int64_t *imap, *iclr;
	int ino;

#ifdef SUN4V
#if 0
XXX
	if (CPU_ISSUN4V) {
		struct upa_reg reg;
		u_int64_t devhandle, devino = INTINO(ihandle);
		u_int64_t sysino;
		int node = -1;
		int i, err;

		for (i = 0; i < sc->sc_nintmap; i++) {
			if (sc->sc_intmap[i].cintr == ihandle) {
				node = sc->sc_intmap[i].cnode;
				break;
			}
		}
		if (node == -1)
			return (NULL);

		if (OF_getprop(node, "reg", &reg, sizeof(reg)) != sizeof(reg))
			return (NULL);
		devhandle = (reg.ur_paddr >> 32) & 0x0fffffff;

		err = hv_intr_devino_to_sysino(devhandle, devino, &sysino);
		if (err != H_EOK)
			return (NULL);

		KASSERT(sysino == INTVEC(sysino));
		ih = bus_intr_allocate(t0, handler, arg, sysino, level,
		    NULL, NULL, what);
		if (ih == NULL)
			return (NULL);

		intr_establish(ih->ih_pil, ih);
		ih->ih_ack = ebus_mainbus_intr_ack;

		err = hv_intr_settarget(sysino, cpus->ci_upaid);
		if (err != H_EOK)
			return (NULL);

		/* Clear pending interrupts. */
		err = hv_intr_setstate(sysino, INTR_IDLE);
		if (err != H_EOK)
			return (NULL);

		err = hv_intr_setenabled(sysino, INTR_ENABLED);
		if (err != H_EOK)
			return (NULL);

		return (ih);
	}
#endif
#endif

	ino = INTINO(ihandle);

	struct pyro_softc *psc = NULL;
	int i;

	for (i = 0; i < pyro_cd.cd_ndevs; i++) {
		device_t dt = pyro_cd.cd_devs[i];
		psc = device_private(dt);
		if (psc && psc->sc_ign == INTIGN(ihandle)) {
			break;
		}
	}
	if (psc == NULL)
		return (NULL);
	
	imap = (uint64_t *)((uintptr_t)bus_space_vaddr(psc->sc_bustag, psc->sc_csrh) + 0x1000);
	iclr = (uint64_t *)((uintptr_t)bus_space_vaddr(psc->sc_bustag, psc->sc_csrh) + 0x1400);
	intrmapptr = &imap[ino];
	intrclrptr = &iclr[ino];
	ino |= INTVEC(ihandle);

	ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	/* Register the map and clear intr registers */
	ih->ih_map = intrmapptr;
	ih->ih_clr = intrclrptr;

	ih->ih_ivec = ihandle;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;
	ih->ih_pending = 0;

	intr_establish(ih->ih_pil, level != IPL_VM, ih);

	if (intrmapptr != NULL) {
		u_int64_t imapval;

		imapval = *intrmapptr;
		imapval |= (1LL << 6);
		imapval |= INTMAP_V;
		*intrmapptr = imapval;
		imapval = *intrmapptr;
		ih->ih_number |= imapval & INTMAP_INR;
	}

	return (ih);
}

#ifdef SUN4V
#if 0
XXX
static void
ebus_mainbus_intr_ack(struct intrhand *ih)
{
	hv_intr_setstate(ih->ih_number, INTR_IDLE);
}
#endif
#endif
