/* $NetBSD: mcclock_isa.c,v 1.20.70.1 2021/05/13 00:47:21 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_isa.c,v 1.20.70.1 2021/05/13 00:47:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>
#include <dev/isa/isavar.h>

#include <alpha/alpha/mcclockvar.h>

static int	mcclock_isa_match(device_t, cfdata_t, void *);
static void	mcclock_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock_isa, sizeof(struct mc146818_softc),
    mcclock_isa_match, mcclock_isa_attach, NULL, NULL);

static void	mcclock_isa_write(struct mc146818_softc *, u_int, u_int);
static u_int	mcclock_isa_read(struct mc146818_softc *, u_int);

static int
mcclock_isa_match(device_t parent, cfdata_t cf, void *aux)
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

static void
mcclock_isa_attach(device_t parent, device_t self, void *aux)
{
	struct mc146818_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_bsh))
		panic("mcclock_isa_attach: couldn't map clock I/O space");

	sc->sc_mcread  = mcclock_isa_read;
	sc->sc_mcwrite = mcclock_isa_write;

	mcclock_attach(sc);
}

static void
mcclock_isa_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

static u_int
mcclock_isa_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
