/*	$NetBSD: cy_pci.c,v 1.8.14.2 2001/01/05 17:36:02 bouyer Exp $	*/

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

static int cy_match_pci __P((struct device *, struct cfdata *, void *));
static void cy_attach_pci  __P((struct device *, struct device *, void *));
static int cy_map_pci __P((struct pci_attach_args *, bus_space_tag_t *,
    bus_space_handle_t *, bus_size_t  *, bus_space_tag_t *,
    bus_space_handle_t *, bus_size_t *));
static void cy_unmap_pci __P((bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_space_tag_t, bus_space_handle_t, bus_size_t));

struct cfattach cy_pci_ca = {
	sizeof(struct cy_softc), cy_match_pci, cy_attach_pci
};

static int
cy_map_pci(pap, iotp, iohp, iosizep, memtp, memhp, memsizep)
	struct pci_attach_args	*pap;
	bus_space_tag_t		*iotp, *memtp;
	bus_space_handle_t	*iohp, *memhp;
	bus_size_t		*iosizep, *memsizep;
{
	int ioh_valid, memh_valid;
	pcireg_t expected;

	/* Map I/O (if possible). */
	ioh_valid = (pci_mapreg_map(pap, 0x14, PCI_MAPREG_TYPE_IO, 0,
	    iotp, iohp, NULL, iosizep) == 0);

	/* Map mem (if possible).  Expected mem type depends on board ID.  */
	expected = PCI_MAPREG_TYPE_MEM;
	switch (PCI_PRODUCT(pap->pa_id)) {
	case PCI_PRODUCT_CYCLADES_CYCLOMY_1:
	case PCI_PRODUCT_CYCLADES_CYCLOM4Y_1:
	case PCI_PRODUCT_CYCLADES_CYCLOM8Y_1:
		expected |= PCI_MAPREG_MEM_TYPE_32BIT_1M;
		break;
	default:
		expected |= PCI_MAPREG_MEM_TYPE_32BIT;
	}
	memh_valid = (pci_mapreg_map(pap, 0x18, expected, 0,
	    memtp, memhp, NULL, memsizep) == 0);

	if (ioh_valid && memh_valid)
		return (1);

	if (ioh_valid)
		bus_space_unmap(*iotp, *iohp, *iosizep);
	if (memh_valid)
		bus_space_unmap(*memtp, *memhp, *memsizep);
	return (0);
}

static void
cy_unmap_pci(iot, ioh, iosize, memt, memh, memsize)
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_size_t iosize, memsize;

{
	bus_space_unmap(iot, ioh, iosize);
	bus_space_unmap(memt, memh, memsize);
}

static int
cy_match_pci(parent, match, aux)
	struct device	*parent;
	struct cfdata	*match;
	void		*aux;
{
	struct pci_attach_args *pap = aux;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_size_t iosize, memsize;

	if (PCI_VENDOR(pap->pa_id) != PCI_VENDOR_CYCLADES)
		return 0;

	switch (PCI_PRODUCT(pap->pa_id)) {
	case PCI_PRODUCT_CYCLADES_CYCLOMY_1:
	case PCI_PRODUCT_CYCLADES_CYCLOMY_2:
	case PCI_PRODUCT_CYCLADES_CYCLOM4Y_1:
	case PCI_PRODUCT_CYCLADES_CYCLOM4Y_2:
	case PCI_PRODUCT_CYCLADES_CYCLOM8Y_1:
	case PCI_PRODUCT_CYCLADES_CYCLOM8Y_2:
		break;
	default:
		return 0;
	}

#ifdef CY_DEBUG
	printf("cy: Found Cyclades PCI device, id = 0x%x\n", pap->pa_id);
#endif

	if (cy_map_pci(pap, &iot, &ioh, &iosize, &memt, &memh, &memsize) == 0)
		return (0);

#if 0
	XXX probably should do something like this, but this code is just
	XXX too broken.

#ifdef CY_DEBUG
	printf("%s: pci mapped mem 0x%lx (size %d), io 0x%x (size %d)\n",
	    sc.sc_dev.dv_xname, memaddr, memsize, iobase, iosize);
#endif

	if ((rv = cy_find(&sc)) == 0)
		printf("%s: PCI Cyclom card with no CD1400s!?\n",
		    sc.sc_dev.dv_xname);
#endif /* 0 */

	cy_unmap_pci(iot, ioh, iosize, memt, memh, memsize);
	return (1);
}


static void
cy_attach_pci(parent, self, aux)
	struct device  *parent, *self;
	void *aux;
{
	struct cy_softc *sc = (void *) self;
	struct pci_attach_args *pap = aux;
	pci_intr_handle_t intrhandle;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_size_t iosize, memsize;
	const char *intrstr;
	int plx_ver;

	sc->sc_bustype = CY_BUSTYPE_PCI;

	if (cy_map_pci(pap, &iot, &ioh, &iosize, &memt, &memh,
	    &memsize) == 0) {
		printf(": unable to map device registers\n");
		return;
	}

	sc->sc_memt = memt;
	sc->sc_bsh = memh;

	if (cy_find(sc) == 0) {
		printf(": cannot find CD1400s\n");
		return;
	}

	/* Set up interrupt handler. */
	if (pci_intr_map(pap, &intrhandle) != 0) {
		printf(": couldn't map PCI interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pap->pa_pc, intrhandle);
	sc->sc_ih = pci_intr_establish(pap->pa_pc, intrhandle, IPL_TTY,
	    cy_intr, sc);
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	if (intrstr != NULL)
		printf(": interrupting at %s\n%s", intrstr, self->dv_xname);

	/* attach the hardware */
	cy_attach(parent, self, aux);

	plx_ver = bus_space_read_1(memt, memh, CY_PLX_VER) & 0x0f;

	/* Enable PCI card interrupts */
	switch (plx_ver) {
	case CY_PLX_9050:
		bus_space_write_2(iot, ioh, CY_PCI_INTENA_9050,
		    bus_space_read_2(iot, ioh, CY_PCI_INTENA_9050) | 0x40);
		break;
	case CY_PLX_9060:
	case CY_PLX_9080:
	default:
		bus_space_write_2(iot, ioh, CY_PCI_INTENA,
		    bus_space_read_2(iot, ioh, CY_PCI_INTENA) | 0x900);
	}
}
