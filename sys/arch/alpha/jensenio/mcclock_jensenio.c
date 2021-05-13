/* $NetBSD: mcclock_jensenio.c,v 1.10.70.1 2021/05/13 00:47:21 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mcclock_jensenio.c,v 1.10.70.1 2021/05/13 00:47:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>
#include <dev/eisa/eisavar.h>
#include <dev/isa/isavar.h>
#include <alpha/jensenio/jenseniovar.h>
#include <alpha/alpha/mcclockvar.h>

struct mcclock_jensenio_softc {
	struct mc146818_softc	sc_mc146818;

	bus_space_handle_t	sc_std_rtc_ioh;
};

static int	mcclock_jensenio_match(device_t, cfdata_t, void *);
static void	mcclock_jensenio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock_jensenio, sizeof(struct mcclock_jensenio_softc),
    mcclock_jensenio_match, mcclock_jensenio_attach, NULL, NULL);

static void	mcclock_jensenio_write(struct mc146818_softc *, u_int, u_int);
static u_int	mcclock_jensenio_read(struct mc146818_softc *, u_int);


static int
mcclock_jensenio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
mcclock_jensenio_attach(device_t parent, device_t self, void *aux)
{
	struct mcclock_jensenio_softc *jsc = device_private(self);
	struct jensenio_attach_args *ja = aux;
	struct mc146818_softc *sc = &jsc->sc_mc146818;

	sc->sc_dev = self;
	sc->sc_bst = ja->ja_iot;
	if (bus_space_map(sc->sc_bst, ja->ja_ioaddr, 0x02, 0,
	    &sc->sc_bsh))
		panic("mcclock_jensenio_attach: couldn't map clock I/O space");

	/*
	 * Map the I/O space normally used by the ISA RTC (port 0x70) so
	 * as to avoid a false match at that address, as well.
	 */
	(void)bus_space_map(sc->sc_bst, 0x70, 0x02, 0,
	    &jsc->sc_std_rtc_ioh);

	sc->sc_mcread  = mcclock_jensenio_read;
	sc->sc_mcwrite = mcclock_jensenio_write;

	mcclock_attach(sc);
}

static void
mcclock_jensenio_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

static u_int
mcclock_jensenio_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	return bus_space_read_1(iot, ioh, 1);
}
