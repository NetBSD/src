/*	$NetBSD: mcclock_isa.c,v 1.14.6.1 2006/04/22 11:37:54 simonb Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_isa.c,v 1.14.6.1 2006/04/22 11:37:54 simonb Exp $");

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <dev/isa/isavar.h>

#define	MCCLOCK_NPORTS	2

int	mcclock_isa_match(struct device *, struct cfdata *, void *);
void	mcclock_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mcclock_isa, sizeof (struct mc146818_softc),
    mcclock_isa_match, mcclock_isa_attach, NULL, NULL);

void	mcclock_isa_write(struct mc146818_softc *, u_int, u_int);
u_int	mcclock_isa_read(struct mc146818_softc *, u_int);


int
mcclock_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	struct mc146818_softc mc146818, *sc;
	unsigned int cra, t1, t2;
	int found;

	found = 0;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	     ia->ia_io[0].ir_addr != 0x70))
		return (0);

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM))
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ))
		return (0);

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISA_UNKNOWN_DRQ))
		return (0);

	sc = &mc146818;
	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, 0x70, MCCLOCK_NPORTS, 0, &sc->sc_bsh))
		return (0);

	/* Supposedly no update in progress after POST; check for this. */
	cra = mcclock_isa_read(sc, MC_REGA);
	if (cra & MC_REGA_UIP)
		goto unmap;

	/* Read from the seconds counter. */
	t1 = FROMBCD(mcclock_isa_read(sc, MC_SEC));
	if (t1 > 59)
		goto unmap;

	/* Wait, then look again. */
	DELAY(1100000);
	t2 = FROMBCD(mcclock_isa_read(sc, MC_SEC));
	if (t2 > 59)
		goto unmap;

#ifdef DEBUG
	printf("mcclock_isa_attach: t1 %02d, t2 %02d\n", t1, t2);
#endif

        /* If [1,2) seconds have passed since, call it a clock. */
	if ((t1 + 1) % 60 == t2 || (t1 + 2) % 60 == t2)
		found = 1;

 unmap:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, MCCLOCK_NPORTS);

	if (found) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_addr = 0x70;
		ia->ia_io[0].ir_size = MCCLOCK_NPORTS;

		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}

	return (found);
}

void
mcclock_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct mc146818_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;

	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_bsh))
		panic("mcclock_isa_attach: couldn't map clock I/O space");

	sc->sc_year0 = 1900;
	sc->sc_mcread = mcclock_isa_read;
	sc->sc_mcwrite = mcclock_isa_write;
	sc->sc_flag = MC146818_BCD;
	mc146818_attach(sc);

	printf("\n");

	(*sc->sc_mcwrite)(sc, MC_REGB, MC_REGB_24HR);

	todr_attach(&sc->sc_handle);
}

void
mcclock_isa_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_isa_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int datum;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	datum = bus_space_read_1(iot, ioh, 1);

	return (datum);
}
