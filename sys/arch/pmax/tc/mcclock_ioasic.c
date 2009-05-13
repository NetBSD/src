/*	$NetBSD: mcclock_ioasic.c,v 1.19.24.1 2009/05/13 17:18:14 jym Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_ioasic.c,v 1.19.24.1 2009/05/13 17:18:14 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <dev/clock_subr.h>

#include <dev/dec/mcclockvar.h>
#include <dev/dec/mcclock_pad32.h>

#include <dev/tc/tcvar.h> 
#include <dev/tc/ioasicvar.h>

static int	mcclock_ioasic_match(struct device *, struct cfdata *,
		    void *);
static void	mcclock_ioasic_attach(struct device *, struct device *,
		    void *);

CFATTACH_DECL(mcclock_ioasic, sizeof (struct mcclock_pad32_softc),
    mcclock_ioasic_match, mcclock_ioasic_attach, NULL, NULL);

static int
mcclock_ioasic_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ioasicdev_attach_args *d = aux;

	if (strcmp("mc146818", d->iada_modname) != 0)
		return (0);

	if (tc_badaddr(d->iada_addr))
		return (0);

	return (1);
}

static void
mcclock_ioasic_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioasicdev_attach_args *ioasicdev = aux;
	struct mcclock_pad32_softc *sc = (struct mcclock_pad32_softc *)self;

	sc->sc_dp = (struct mcclock_pad32_clockdatum *)ioasicdev->iada_addr;

	/* Attach MI driver, using busfns with TC-style register padding */
	mcclock_attach(&sc->sc_mcclock, &mcclock_pad32_busfns);
}
