/* $NetBSD: mcclock_ioasic.c,v 1.16.2.1 2012/04/17 00:05:58 yamt Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

__KERNEL_RCSID(0, "$NetBSD: mcclock_ioasic.c,v 1.16.2.1 2012/04/17 00:05:58 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>                   /* XXX */

#include <alpha/alpha/mcclockvar.h>

struct mcclock_ioasic_clockdatum {
	u_char	datum;
	char	pad[3];
};

struct mcclock_ioasic_softc {
	struct mc146818_softc	sc_mc146818;

	struct mcclock_ioasic_clockdatum *sc_dp;
};

int	mcclock_ioasic_match(device_t, cfdata_t, void *);
void	mcclock_ioasic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock_ioasic, sizeof(struct mcclock_ioasic_softc),
    mcclock_ioasic_match, mcclock_ioasic_attach, NULL, NULL);

void	mcclock_ioasic_write(struct mc146818_softc *, u_int, u_int);
u_int	mcclock_ioasic_read(struct mc146818_softc *, u_int);

int
mcclock_ioasic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ioasicdev_attach_args *d = aux;

	if (strncmp("TOY_RTC ", d->iada_modname, TC_ROM_LLEN))
		return (0);

	return (1);
}

void
mcclock_ioasic_attach(device_t parent, device_t self, void *aux)
{
	struct mcclock_ioasic_softc *isc = device_private(self);
	struct ioasicdev_attach_args *ioasicdev = aux;
	struct mc146818_softc *sc = &isc->sc_mc146818;

	/* XXX no bus_space(9) for TURBOchannel yet */
	isc->sc_dp = (void *)ioasicdev->iada_addr;

	sc->sc_dev = self;
	sc->sc_mcread = mcclock_ioasic_read;
	sc->sc_mcwrite = mcclock_ioasic_write;

	/* call alpha common mcclock attachment */
	mcclock_attach(sc);
}

void
mcclock_ioasic_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	struct mcclock_ioasic_softc *isc = (void *)sc;

	isc->sc_dp[reg].datum = datum;
}

u_int
mcclock_ioasic_read(struct mc146818_softc *sc, u_int reg)
{
	struct mcclock_ioasic_softc *isc = (void *)sc;

	return isc->sc_dp[reg].datum;
}
