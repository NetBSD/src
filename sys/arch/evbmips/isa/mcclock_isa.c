/*	$NetBSD: mcclock_isa.c,v 1.2 2002/04/09 03:40:16 simonb Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_isa.c,v 1.2 2002/04/09 03:40:16 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/ic/mc146818reg.h>

#include <evbmips/evbmips/clockvar.h>
#include <evbmips/dev/mcclockvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

struct mcclock_isa_softc {
	struct mcclock_softc	sc_mcclock;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static int	mcclock_isa_match(struct device *, struct cfdata *, void *);
static void	mcclock_isa_attach(struct device *, struct device *, void *);

struct cfattach mcclock_isa_ca = {
	sizeof (struct mcclock_isa_softc), mcclock_isa_match,
	    mcclock_isa_attach, 
};

static void	mcclock_isa_write(struct mcclock_softc *, u_int, u_int);
static u_int	mcclock_isa_read(struct mcclock_softc *, u_int);

const struct mcclock_busfns mcclock_isa_busfns = {
	mcclock_isa_write, mcclock_isa_read,
};

static int
mcclock_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISACF_PORT_DEFAULT &&
	     ia->ia_io[0].ir_addr != IO_RTC))
		return (0);

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISACF_IOMEM_DEFAULT))
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISACF_IRQ_DEFAULT))
		return (0);

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISACF_DRQ_DEFAULT))
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
	struct mcclock_isa_softc *sc = (struct mcclock_isa_softc *)self;

	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_ioh))
		panic("mcclock_isa_attach: couldn't map clock I/O space");

	mcclock_attach(&sc->sc_mcclock, &mcclock_isa_busfns);
}

static void
mcclock_isa_write(struct mcclock_softc *mcsc, u_int reg, u_int datum)
{
	struct mcclock_isa_softc *sc = (struct mcclock_isa_softc *)mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

static u_int
mcclock_isa_read(struct mcclock_softc *mcsc, u_int reg)
{
	struct mcclock_isa_softc *sc = (struct mcclock_isa_softc *)mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
