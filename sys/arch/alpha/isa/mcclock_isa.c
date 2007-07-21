/* $NetBSD: mcclock_isa.c,v 1.16 2007/07/21 11:59:57 tsutsui Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_isa.c,v 1.16 2007/07/21 11:59:57 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>
#include <dev/isa/isavar.h>

#include <alpha/alpha/mcclockvar.h>

int	mcclock_isa_match(struct device *, struct cfdata *, void *);
void	mcclock_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mcclock_isa, sizeof(struct mc146818_softc),
    mcclock_isa_match, mcclock_isa_attach, NULL, NULL);

void	mcclock_isa_write(struct mc146818_softc *, u_int, u_int);
u_int	mcclock_isa_read(struct mc146818_softc *, u_int);

int
mcclock_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

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

	if (bus_space_map(ia->ia_iot, 0x70, 0x2, 0, &ioh))
		return (0);

	bus_space_unmap(ia->ia_iot, ioh, 0x2);

	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = 0x70;
	ia->ia_io[0].ir_size = 0x02;

	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return (1);
}

void
mcclock_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct isa_attach_args *ia = aux;
	struct mc146818_softc *sc = (void *)self;

	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_bsh))
		panic("mcclock_isa_attach: couldn't map clock I/O space");

	sc->sc_mcread  = mcclock_isa_read;
	sc->sc_mcwrite = mcclock_isa_write;

	mcclock_attach(sc);
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
