/*	$NetBSD: cy_isa.c,v 1.12.2.3 2002/02/28 04:13:39 nathanw Exp $	*/

/*
 * cy.c
 *
 * Driver for Cyclades Cyclom-8/16/32 multiport serial cards
 * (currently not tested with Cyclom-32 cards)
 *
 * Timo Rossi, 1996
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cy_isa.c,v 1.12.2.3 2002/02/28 04:13:39 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <dev/ic/cd1400reg.h>
#include <dev/ic/cyreg.h>
#include <dev/ic/cyvar.h>

int	cy_isa_probe(struct device *, struct cfdata *, void *);
void	cy_isa_attach(struct device *, struct device *, void *);

struct cfattach cy_isa_ca = {
	sizeof(struct cy_softc), cy_isa_probe, cy_isa_attach
};

int
cy_isa_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	struct cy_softc sc;
	int found;

	if (ia->ia_niomem < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	memcpy(&sc.sc_dev, match, sizeof(struct device));

	sc.sc_memt = ia->ia_memt;
	sc.sc_bustype = CY_BUSTYPE_ISA;

	/* Disallow wildcarded memory address. */
	if (ia->ia_iomem[0].ir_addr == ISACF_IOMEM_DEFAULT)
		return 0;
	if (ia->ia_irq[0].ir_irq == ISACF_IRQ_DEFAULT)
		return 0;

	if (bus_space_map(ia->ia_memt, ia->ia_iomem[0].ir_addr, CY_MEMSIZE, 0,
	    &sc.sc_bsh) != 0)
		return 0;

	found = cy_find(&sc);

	bus_space_unmap(ia->ia_memt, sc.sc_bsh, CY_MEMSIZE);

	if (found) {
		ia->ia_niomem = 1;
		ia->ia_iomem[0].ir_size = CY_MEMSIZE;

		ia->ia_nirq = 1;

		ia->ia_nio = 0;
		ia->ia_ndrq = 0;
	}
	return (found);
}

void
cy_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct cy_softc *sc = (void *) self;
	struct isa_attach_args *ia = aux;

	sc->sc_memt = ia->ia_memt;
	sc->sc_bustype = CY_BUSTYPE_ISA;

	printf(": Cyclades-Y multiport serial\n");

	if (bus_space_map(ia->ia_memt, ia->ia_iomem[0].ir_addr, CY_MEMSIZE, 0,
	    &sc->sc_bsh) != 0) {
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (cy_find(sc) == 0) {
		printf("%s: unable to find CD1400s\n", sc->sc_dev.dv_xname);
		return;
	}

	cy_attach(sc);

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_EDGE, IPL_TTY, cy_intr, sc);
	if (sc->sc_ih == NULL)
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
}
