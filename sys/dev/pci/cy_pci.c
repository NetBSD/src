/*	$NetBSD: cy_pci.c,v 1.1 1996/09/24 17:59:33 christos Exp $	*/

/*
 * cy_pci.c
 *
 * Driver for Cyclades Cyclom-8/16/32 multiport serial cards
 * (currently not tested with Cyclom-32 cards)
 *
 * Timo Rossi, 1996
 *
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/cd1400reg.h>
#include <dev/ic/cyreg.h>
#include <dev/ic/cyvar.h>

static int cy_probe_pci __P((struct device *, void *, void *));
static void cy_attach_pci  __P((struct device *, struct device *, void *));
static int cy_map_pci __P((struct pci_attach_args *, struct cy_softc *,
    bus_io_handle_t *, bus_mem_size_t  *, bus_io_size_t *));
static void cy_unmap_pci __P((struct cy_softc *, bus_io_handle_t,
    bus_mem_size_t, bus_io_size_t));

struct cfattach cy_pci_ca = {
	sizeof(struct cy_softc), cy_probe_pci, cy_attach_pci
};

static int
cy_map_pci(pa, sc, ioh, iosize, memsize)
	struct pci_attach_args *pa;
	struct cy_softc *sc;
	bus_io_handle_t *ioh;
	bus_mem_size_t  *memsize;
	bus_io_size_t *iosize;
{
	bus_io_addr_t iobase;
	bus_mem_addr_t  memaddr;
	int cacheable;

	if (pci_mem_find(pa->pa_pc, pa->pa_tag, 0x18, &memaddr, memsize,
	    &cacheable) != 0) {
		printf("%s: can't find PCI card memory", sc->sc_dev.dv_xname);
		return 0;
	}
	/* map the memory (non-cacheable) */
	if (bus_mem_map(sc->sc_bc, memaddr, *memsize, 0, &sc->sc_memh) != 0) {
		printf("%s: couldn't map PCI memory region\n",
		    sc->sc_dev.dv_xname);
		return 0;
	}
	/* the PCI Cyclom IO space is only used for enabling interrupts */
	if (pci_io_find(pa->pa_pc, pa->pa_tag, 0x14, &iobase, iosize) != 0) {
		printf("%s: couldn't find PCI io region\n",
		    sc->sc_dev.dv_xname);
		goto unmapmem;
	}
	if (bus_io_map(sc->sc_bc, iobase, *iosize, ioh) != 0) {
		printf("%s: couldn't map PCI io region\n", sc->sc_dev.dv_xname);
		goto unmapmem;
	}
	return 1;

unmapio:
	bus_io_unmap(sc->sc_bc, *ioh, *iosize);
unmapmem:
	bus_mem_unmap(sc->sc_bc, sc->sc_memh, *memsize);
	return 0;
}

static void
cy_unmap_pci(sc, ioh, iosize, memsize)
	struct cy_softc *sc;
	bus_io_handle_t ioh;
	bus_io_size_t iosize;
	bus_mem_size_t  memsize;

{
	bus_io_unmap(sc->sc_bc, ioh, iosize);
	bus_mem_unmap(sc->sc_bc, sc->sc_memh, memsize);
}

static int
cy_probe_pci(parent, match, aux)
	struct device  *parent;
	void           *match, *aux;
{
	vm_offset_t v_addr, p_addr;
	struct pci_attach_args *pa = aux;
	bus_io_handle_t ioh;
	bus_mem_size_t  memsize;
	bus_io_size_t iosize;
	int rv = 0;
	struct cy_softc sc;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_CYCLADES)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_CYCLADES_CYCLOMY_1:
		break;
	case PCI_PRODUCT_CYCLADES_CYCLOMY_2:
		break;
	default:
		return 0;
	}

#ifdef CY_DEBUG
	printf("cy: Found Cyclades PCI device, id = 0x%x\n", pa->pa_id);
#endif
	memcpy(&sc.sc_dev, match, sizeof(struct device));

	sc.sc_bc = pa->pa_bc;
	sc.sc_bustype = CY_BUSTYPE_PCI;

	if (cy_map_pci(pa, &sc, &ioh, &iosize, &memsize) == 0)
		return 0;

#ifdef CY_DEBUG
	printf("%s: pci mapped mem 0x%lx (size %d), io 0x%x (size %d)\n",
	    sc.sc_dev.dv_xname, memaddr, memsize, iobase, iosize);
#endif

	if ((rv = cy_find(&sc)) == 0)
		printf("%s: PCI Cyclom card with no CD1400s!?\n",
		    sc.sc_dev.dv_xname);

	cy_unmap_pci(&sc, ioh, iosize, memsize);
	return rv;
}


static void
cy_attach_pci(parent, self, aux)
	struct device  *parent, *self;
	void *aux;
{
	struct cy_softc *sc = (void *) self;
	pci_intr_handle_t intrhandle;
	bus_io_handle_t ioh;
	bus_mem_size_t  memsize;
	bus_io_size_t iosize;
	struct pci_attach_args *pa = aux;

	sc->sc_bc = pa->pa_bc;
	sc->sc_bustype = CY_BUSTYPE_PCI;

	if (cy_map_pci(pa, sc, &ioh, &iosize, &memsize) == 0)
		panic("%s: mapping failed", sc->sc_dev.dv_xname);

	if (cy_find(sc) == 0)
		panic("%s: Cannot find card", sc->sc_dev.dv_xname);

	cy_attach(parent, self, aux);

	/* Enable PCI card interrupts */
	bus_io_write_2(sc->sc_bc, ioh, CY_PCI_INTENA,
	    bus_io_read_2(sc->sc_bc, ioh, CY_PCI_INTENA) | 0x900);

	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &intrhandle) != 0)
		panic("%s: couldn't map PCI interrupt", sc->sc_dev.dv_xname);


	sc->sc_ih = pci_intr_establish(pa->pa_pc, intrhandle,
	    IPL_TTY, cy_intr, sc);

	if (sc->sc_ih == NULL)
		panic("%s: couldn't establish interrupt", sc->sc_dev.dv_xname);
}
