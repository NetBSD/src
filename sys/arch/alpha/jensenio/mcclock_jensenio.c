/* $NetBSD: mcclock_jensenio.c,v 1.1.2.2 2000/07/12 20:59:15 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_jensenio.c,v 1.1.2.2 2000/07/12 20:59:15 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/dec/clockvar.h>
#include <dev/dec/mcclockvar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/eisa/eisavar.h>
#include <dev/isa/isavar.h>
#include <alpha/jensenio/jenseniovar.h>

struct mcclock_jensenio_softc {
	struct mcclock_softc	sc_mcclock;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	bus_space_handle_t	sc_std_rtc_ioh;
};

int	mcclock_jensenio_match(struct device *, struct cfdata *, void *);
void	mcclock_jensenio_attach(struct device *, struct device *, void *);

struct cfattach mcclock_jensenio_ca = {
	sizeof (struct mcclock_jensenio_softc), mcclock_jensenio_match,
	    mcclock_jensenio_attach, 
};

void	mcclock_jensenio_write(struct mcclock_softc *, u_int, u_int);
u_int	mcclock_jensenio_read(struct mcclock_softc *, u_int);

const struct mcclock_busfns mcclock_jensenio_busfns = {
	mcclock_jensenio_write, mcclock_jensenio_read,
};

int
mcclock_jensenio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
mcclock_jensenio_attach(struct device *parent, struct device *self, void *aux)
{
	struct jensenio_attach_args *ja = aux;
	struct mcclock_jensenio_softc *sc = (void *) self;

	sc->sc_iot = ja->ja_iot;
	if (bus_space_map(sc->sc_iot, ja->ja_ioaddr, 0x02, 0,
	    &sc->sc_ioh))
		panic("mcclock_jensenio_attach: couldn't map clock I/O space");

	/*
	 * Map the I/O space normally used by the ISA RTC (port 0x70) so
	 * as to avoid a false match at that address, as well.
	 */
	(void) bus_space_map(sc->sc_iot, 0x70, 0x02, 0,
	    &sc->sc_std_rtc_ioh);

	mcclock_attach(&sc->sc_mcclock, &mcclock_jensenio_busfns);
}

void
mcclock_jensenio_write(struct mcclock_softc *mcsc, u_int reg, u_int datum)
{
	struct mcclock_jensenio_softc *sc = (void *) mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_jensenio_read(struct mcclock_softc *mcsc, u_int reg)
{
	struct mcclock_jensenio_softc *sc = (void *) mcsc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
