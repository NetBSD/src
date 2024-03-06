/* $NetBSD: mcclock_gbus.c,v 1.3 2024/03/06 05:44:44 thorpej Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_gbus.c,v 1.3 2024/03/06 05:44:44 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/gbus/gbusvar.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <alpha/alpha/mcclockvar.h>

#include "ioconf.h"

static int	mcclock_gbus_match(device_t, cfdata_t, void *);
static void	mcclock_gbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock_gbus, sizeof(struct mc146818_softc),
    mcclock_gbus_match, mcclock_gbus_attach, NULL, NULL);

static void	mcclock_gbus_write(struct mc146818_softc *, u_int, u_int);
static u_int	mcclock_gbus_read(struct mc146818_softc *, u_int);

static int
mcclock_gbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gbus_attach_args *ga = aux;

	if (strcmp(ga->ga_name, mcclock_cd.cd_name))
		return (0);
	return (1);
}

static void
mcclock_gbus_attach(device_t parent, device_t self, void *aux)
{
	struct mc146818_softc *sc = device_private(self);
	struct gbus_attach_args *ga = aux;

	sc->sc_dev = self;
	sc->sc_bst = ga->ga_iot;

	if (bus_space_map(sc->sc_bst, ga->ga_offset, MC_NREGS+MC_NVRAM_SIZE,
			  0, &sc->sc_bsh) != 0) {
		panic("mcclock_gbus_attach: couldn't map clock I/O space");
	}

	sc->sc_mcread  = mcclock_gbus_read;
	sc->sc_mcwrite = mcclock_gbus_write;

	mcclock_attach(sc);
}

static void
mcclock_gbus_write(struct mc146818_softc *sc, u_int reg, u_int val)
{
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, reg, (uint8_t)val);
}

static u_int
mcclock_gbus_read(struct mc146818_softc *sc, u_int reg)
{
	return bus_space_read_1(sc->sc_bst, sc->sc_bsh, reg);
}
