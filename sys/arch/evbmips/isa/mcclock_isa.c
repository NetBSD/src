/*	$NetBSD: mcclock_isa.c,v 1.9.46.1 2008/01/10 23:43:15 bouyer Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_isa.c,v 1.9.46.1 2008/01/10 23:43:15 bouyer Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

/*
 * Note the Algorithmics PMON firmware uses a different year base.
 */
#define	ALGOR_YEAR_ZERO		1920

static int	mcclock_isa_match(struct device *, struct cfdata *, void *);
static void	mcclock_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mcclock_isa, sizeof (struct mc146818_softc),
    mcclock_isa_match, mcclock_isa_attach, NULL, NULL);

static void	mcclock_isa_write(struct mc146818_softc *, u_int, u_int);
static u_int	mcclock_isa_read(struct mc146818_softc *, u_int);

static int
mcclock_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	     ia->ia_io[0].ir_addr != IO_RTC))
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

	if (bus_space_map(ia->ia_iot, IO_RTC, 0x2, 0, &ioh))
		return (0);

	bus_space_unmap(ia->ia_iot, ioh, 0x2);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = IO_RTC;
	ia->ia_io[0].ir_size = 0x02;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return (1);
}

static void
mcclock_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct isa_attach_args *ia = aux;
	struct mc146818_softc *sc = (struct mc146818_softc *)self;

	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_bsh))
		panic("mcclock_isa_attach: couldn't map clock I/O space");

	sc->sc_year0 = ALGOR_YEAR_ZERO;
	sc->sc_flag = MC146818_NO_CENT_ADJUST;
	sc->sc_mcread = mcclock_isa_read;
	sc->sc_mcwrite = mcclock_isa_write;
	sc->sc_getcent = NULL;
	sc->sc_setcent = NULL;

	/*
	 * Turn interrupts off, just in case.  Need to leave the SQWE
	 * set, because that's the DRAM refresh signal on Rev. B boards.
	 */
	mcclock_isa_write(sc, MC_REGB, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	mc146818_attach(sc);
	aprint_normal("\n");
}

void
mcclock_isa_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_isa_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
